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

#include <iostream>
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

blogi::Template::Template(blogi::TemplateConfig& config){
    _Config=config;

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
                _PublicFiles.insert(std::pair<std::string,std::string>(std::string("/theme/public/").append(direntStruct->d_name),buf));

                fs.close();
            }
        }
        closedir(directory);
    }

}

blogi::Template::~Template(){

}

void blogi::Template::renderPage(const char *name,libhtmlpp::HtmlPage& page, libhtmlpp::HtmlElement& index){
    std::string htmlfile=_Config.Theme;
    htmlfile.append("/");
    htmlfile.append(name);
    index=page.loadFile(htmlfile.c_str());
}

bool blogi::Template::Controller(netplus::con *curcon,libhttppp::HttpRequest *req){
    std::string publicf = req->getRequestURL();
    if(publicf.length() >strlen(_Config.config->getprefix()) && publicf.compare(strlen(_Config.config->getprefix()),13,"/theme/public/",13)==0){
        for(auto curfile=_PublicFiles.begin(); curfile!=_PublicFiles.end(); curfile++){
            if(publicf.compare(strlen(_Config.config->getprefix()),curfile->first.length(),curfile->first,0,curfile->first.length())==0){
                libhttppp::HttpResponse resp;
                resp.setVersion(HTTPVERSION(1.1));
                resp.setState(HTTP200);

                std::string type=curfile->first.substr(curfile->first.rfind(".")+1,
                                                       curfile->first.length()-(curfile->first.rfind(".")+1));

                if(type=="png" || type=="jpg" || type=="webp"){
                    resp.setContentType(std::string("image/").append(type).c_str());
                }else if(type=="css" || type=="html"){
                    resp.setContentType(std::string("text/").append(type).c_str());
                }else if(type=="js"){
                    resp.setContentType("application/javascript");
                }else{
                    resp.setContentType("application/octet-stream");
                }

                resp.send(curcon,curfile->second.c_str(),curfile->second.length());
                return true;
            }
        }
        libhttppp::HTTPException excep;
        excep[libhttppp::HTTPException::Error] << "template file nod found!";
        throw excep;

    }
    return false;
}

void blogi::Template::printSite(std::string &output,libhtmlpp::HtmlElement index,const char *crrurl,bool login,const char *meta){
    try{
        char url[512];

        std::string sessid;

        libhtmlpp::HtmlElement *head,*header,*main, *footernav;
        head=index.getElementbyTag("head");

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
        footernav = index.getElementbyID("footernav");
        libhtmlpp::HtmlElement contentd("div");
        libhtmlpp::HtmlString footerancor;
        footerancor << "<a class=\"footer\" href=\"" << _Config.config->buildurl("staticpage/impressum",url,512) << "\" >Impressum</a>";
        if(!login){
            footerancor << "<a class=\"footer\" href=\"" << _Config.config->buildurl("login",url,512) << "\">Login</a> ";
        }else{
            footerancor << "<a class=\"footer\" href=\"" << _Config.config->buildurl("logout",url,512) << "\">Logout</a>"
            << "<a class=\"footer\" href=\"" << _Config.config->buildurl("settings",url,512) << "\">Settings</a>";
        }
        if(footernav)
            footernav->appendChild(footerancor.parse());

        libhtmlpp::print(&index,nullptr,output);
    }catch(libhtmlpp::HTMLException &e){
        libhttppp::HTTPException excep;
        excep[libhttppp::HTTPException::Error] << e.what();
        throw excep;
    }

}





