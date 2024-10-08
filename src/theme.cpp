/*******************************************************************************
 * Copyright (c) 2021, Jan Koester jan.koester@gmx.net
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 * Neither the name of the <organization> nor the
 *      names of its contributors may be used to endorse or promote products
 *      derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *******************************************************************************/

#include <algorithm>
#include <iostream>
#include <cstring>
#include <fstream>
#include <compare>

#include <dirent.h>
#include <dlfcn.h>
#include <unistd.h>
#include <sys/stat.h>

#include <httppp/exception.h>

#include <htmlpp/exception.h>

#include "theme.h"
#include "conf.h"
#include "compress.h"


blogi::Template::Template(blogi::TemplateConfig& config,bool formated){
    _Config=config;
    _Formated=formated;

    DIR *directory = opendir(std::string(_Config.Theme).append("/public").c_str());

    if (directory != nullptr) {
        struct dirent *direntStruct;
        while((direntStruct = readdir(directory))) {
            std::string buf;
            char tmp[BLOCKSIZE];
            std::ifstream fs;
            std::string fpath=std::string(_Config.Theme).append("/public");
            fpath.append("/");
            fpath.append(direntStruct->d_name);

            struct stat curstat;
            stat(fpath.c_str(),&curstat);

            if(S_ISREG(curstat.st_mode)!=0){
                try{
                    fs.open(fpath);
                }catch(std::exception &e){
                    continue;
                }

                while (fs.good()) {
                    fs.read(tmp,BLOCKSIZE);
                    buf.append(tmp,fs.gcount());
                }
                TemplateFiles tfile;

                tfile.Path=std::string("/theme/public/").append(direntStruct->d_name);
                tfile.Content=buf;

                std::string fname=direntStruct->d_name;
                tfile.Ending=fname.substr(fname.rfind(".")+1,fname.length()-(fname.rfind(".")+1));

                if(tfile.Ending=="png" || tfile.Ending=="jpg" || tfile.Ending=="webp" || tfile.Ending=="ico"){
                    tfile.Type=TemplateFilesTypes::IMAGE;
                }else if(tfile.Ending=="css" || tfile.Ending=="html"){
                    compress<std::string>(tfile.Content,tfile.Compressed);
                    tfile.Type=TemplateFilesTypes::TEXT;
                }else if(tfile.Ending=="js"){
                    compress<std::string>(tfile.Content,tfile.Compressed);
                    tfile.Type=TemplateFilesTypes::JAVASCRIPT;

                }else{
                    tfile.Type=TemplateFilesTypes::GENERIC;
                }

                _PublicFiles.push_back(tfile);

                fs.close();
            }
        }
        closedir(directory);
    }

}

blogi::Template::~Template(){

}

void blogi::Template::renderPage(const int tid,const char *name,libhtmlpp::HtmlPage& page, libhtmlpp::HtmlElement &index){
    std::string htmlfile=_Config.Theme;
    htmlfile.append("/");
    htmlfile.append(name);

    std::ifstream fhfile(htmlfile);
    std::vector<char> temp,data;

    if (fhfile.is_open()){
        while ( fhfile.good() ){
            temp.push_back(fhfile.get());
        }
        fhfile.close();
    }else{
        return;
    }

    auto pos=temp.end(),epos=temp.begin();

    do{
        pos=std::find(epos, temp.end(),'$');
        std::copy(epos,pos,std::inserter<std::vector<char>>(data,data.end()));
        std::copy(_Config.config->getprefix(),_Config.config->getprefix()+strlen(_Config.config->getprefix()),
                  std::inserter<std::vector<char>>(data,data.end()));
        epos=std::find(pos, temp.end(),'}');
        ++epos;
    }while( pos != temp.end() );

    data.push_back('\0');

    page.loadString(index,data.data());

}

bool blogi::Template::Controller(const int tid,libhttppp::HttpRequest *req){
    std::string publicf = req->getRequestURL();
    if(publicf.length() >strlen(_Config.config->getprefix()) && publicf.compare(strlen(_Config.config->getprefix()),13,"/theme/public/",13)==0){
        for(auto curfile=_PublicFiles.begin(); curfile!=_PublicFiles.end(); curfile++){
            if(publicf.compare(strlen(_Config.config->getprefix()),curfile->Path.length(),curfile->Path,0,curfile->Path.length())==0){
                libhttppp::HttpResponse resp;
                resp.setVersion(HTTPVERSION(1.1));
                resp.setState(HTTP200);
                *resp.setData("cache-control") << "max-age=31536000";

                if(curfile->Type==TemplateFilesTypes::IMAGE){
                    resp.setContentType(std::string("image/").append(curfile->Ending).c_str());
                }else if(curfile->Type==TemplateFilesTypes::TEXT){
                    resp.setContentType(std::string("text/").append(curfile->Ending).c_str());
                }else if(curfile->Type==TemplateFilesTypes::JAVASCRIPT){
                    resp.setContentType("application/javascript");
                }else if(curfile->Type==TemplateFilesTypes::GENERIC){
                    resp.setContentType("application/octet-stream");
                }

                if( curfile->Compressed.length() >0 && req->getData("accept-encoding")
                    && strstr(req->getData(req->getData("accept-encoding")),"br") ){
                    *resp.setData("content-encoding") << "br";
                    resp.setContentLength(curfile->Compressed.length());
                    resp.send(req,nullptr,-1);
                }else{
                    resp.setContentLength(curfile->Content.length());
                    resp.send(req,nullptr,-1);
                }
                req->SendData.pos=0;
                return true;
            }
        }
        libhttppp::HTTPException excep;
        excep[libhttppp::HTTPException::Error] << "template file nod found!";
        throw excep;

    }
    return false;
}


bool blogi::Template::Response(const int tid,libhttppp::HttpRequest *req){
    std::string publicf = req->getRequestURL();
    if(publicf.length() >strlen(_Config.config->getprefix()) && publicf.compare(strlen(_Config.config->getprefix()),13,"/theme/public/",13)==0){
        for(auto curfile=_PublicFiles.begin(); curfile!=_PublicFiles.end(); curfile++){
            if(publicf.compare(strlen(_Config.config->getprefix()),curfile->Path.length(),curfile->Path,0,curfile->Path.length())==0){
                if(req->SendData.pos  < curfile->Content.length() ){
                    if( curfile->Compressed.length() >0 && req->getData("accept-encoding")
                        && strstr(req->getData(req->getData("accept-encoding")),"br") ){
                        size_t len = BLOCKSIZE < curfile->Compressed.length()-req->SendData.pos ?
                                    BLOCKSIZE : curfile->Compressed.length()-req->SendData.pos;
                        req->SendData.append(curfile->Compressed.substr(req->SendData.pos,len).c_str(),len);
                        req->SendData.pos+=len;
                    }else{
                        size_t len = BLOCKSIZE < curfile->Content.length()-req->SendData.pos ?
                                    BLOCKSIZE : curfile->Content.length()-req->SendData.pos;
                        req->SendData.append(curfile->Content.substr(req->SendData.pos,len).c_str(),len);
                        req->SendData.pos+=len;
                    }
                    return true;
                }
            }
        }

    }
    return false;
}

void blogi::Template::printSite(const int tid,libhtmlpp::HtmlString &output,libhtmlpp::HtmlElement *index,const char *crrurl,bool login,const char *meta){
    try{
        char url[512];

        std::string sessid;

        libhtmlpp::HtmlElement *head,*footernav;
        head=index->getElementbyTag("head");

        std::string hostpath;

        if(meta && head){
            libhtmlpp::HtmlElement des("meta");
            des.setAttribute("name","description");
            des.setAttribute("content",meta);
            head->appendChild(&des);

            libhtmlpp::HtmlElement thumbnail("meta");
            thumbnail.setAttribute("property","og:image");
            thumbnail.setAttribute("content",std::string(_Config.config->getsiteurl()).append(_Config.config->buildurl("theme/public/thumbnail.webp",url,512)).c_str());
            head->appendChild(&thumbnail);

            libhtmlpp::HtmlElement baseurl("meta");
            baseurl.setAttribute("property","og:url");
            baseurl.setAttribute("content",std::string(_Config.config->getsiteurl()).append(crrurl).c_str());
            head->appendChild(&baseurl);


        }
        footernav = index->getElementbyID("footernav");
        libhtmlpp::HtmlString footerancor;
        footerancor << "<div><a class=\"footer\" href=\"" << _Config.config->buildurl("staticpage/impressum",url,512) << "\" >Impressum</a>";
        if(!login){
            footerancor << "<a class=\"footer\" href=\"" << _Config.config->buildurl("login",url,512) << "\">Login</a>";
        }else{
            footerancor << "<a class=\"footer\" href=\"" << _Config.config->buildurl("logout",url,512) << "\">Logout</a>"
            << "<a class=\"footer\" href=\"" << _Config.config->buildurl("settings",url,512) << "\">Settings</a>";
        }
        footerancor << "</div>";

        if(footernav)
            footernav->appendChild(footerancor.parse());

        libhtmlpp::print(index,output,_Formated);
    }catch(libhtmlpp::HTMLException &e){
        libhttppp::HTTPException excep;
        excep[libhttppp::HTTPException::Error] << e.what();
        throw excep;
    }

}





