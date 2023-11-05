/*******************************************************************************
 * Copyright (c) 2023, Jan Koester jan.koester@gmx.net
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

#include <htmlpp/html.h>

#include <conf.h>

#include "static.h"
#include "conf.h"

blogi::StaticPage::StaticPage(){
}

blogi::StaticPage::~StaticPage(){
}

void blogi::StaticPage::initPlugin(){
}

bool blogi::StaticPage::haveSettings(){
    return true;
}

void blogi::StaticPage::Settings(libhttppp::HttpRequest* req, libhtmlpp::HtmlString& setdiv){
    char url[512];
    std::string surl,curl=req->getRequestURL();
    size_t urlen = curl.length();
    size_t prelen = strlen(Args->config->buildurl("settings/staticpage/",url,512));
    if(prelen < urlen){
        size_t send=0,sublen=urlen-prelen;

        for(size_t i =0; i<=sublen; ++i){
            switch(curl[prelen+i]){
                case '\0':
                    send=i;
                    goto SENDFIND;
                case '/':
                    send=i;
                    goto SENDFIND;
                default:
                    continue;
            }
        }
        goto SETTINGSINDEX;
SENDFIND:
        surl=curl.substr(prelen,send);

        std::cout << surl << std::endl;

        if(surl=="newpage"){
            newPage(req,setdiv);
            return;
        }else if(surl=="delpage"){
            delPage(req,setdiv);
            return;
        }else if(surl=="editpage"){
            editPage(req,setdiv);
            return;
        }

    }
SETTINGSINDEX:
    blogi::SQL sql;
    blogi::DBResult res;

    sql << "select id,url from static_content";

    int scount=Args->database->exec(&sql,res);

    setdiv << "<div id=\"staticsettings\"><span>Statische Seiten</span><table>";
    setdiv << "<tr><th>ID</th><th>URL</th><th>Options</th></tr>";
    for(int i=0; i<scount; ++i){
        setdiv << "<tr><td>" << res[i][0] << "</td><td>"<< Args->config->buildurl("staticpage/",url,512) << res[i][1]
               << "</td>" << "<td><a href=\"" << Args->config->buildurl("settings/staticpage/editpage?id=",url,512) << res[i][0] << "\">Edit</a></td>"
               << "<td><a href=\"" << Args->config->buildurl("settings/staticpage/delpage?id=",url,512) << res[i][0] << "\">Remove</a></td>" <<"</tr>";
    }
    setdiv << "</table><a href=\"" << Args->config->buildurl("settings/staticpage/newpage",url,512) << "\" >Neue Seite Erstellen</a></div>";
}

void blogi::StaticPage::newPage(libhttppp::HttpRequest* req, libhtmlpp::HtmlString& setdiv){
    setdiv << "<div id=\"staticsettings\">"
           << "<span>Statische Seiten</span>"
           << "<span>Neue Seite</span>"
           << "</div>";
}

void blogi::StaticPage::delPage(libhttppp::HttpRequest* req, libhtmlpp::HtmlString& setdiv){
}

void blogi::StaticPage::editPage(libhttppp::HttpRequest* req, libhtmlpp::HtmlString& setdiv){
    char url[512];
    int id=-1;
    blogi::SQL sql;
    blogi::DBResult res;

    libhttppp::HttpForm form;
    form.parse(req);

    for(libhttppp::HttpForm::UrlcodedFormData *cdat=form.getUrlcodedFormData(); cdat; cdat=cdat->nextUrlcodedFormData()){
        if(strcmp(cdat->getKey(),"id")==0)
            id=atoi(cdat->getValue());
    }

    if(id<0){
        return;
    }

    try {
        sql.clear();

        for (libhttppp::HttpForm::MultipartFormData* curformdat = form.getMultipartFormData(); curformdat; curformdat = curformdat->nextMultipartFormData()) {
            libhttppp::HttpForm::MultipartFormData::ContentDisposition* curctdisp = curformdat->getContentDisposition();
            if(strcmp(curctdisp->getName(),"url")==0){
                sql<< "update static_content set url='"; sql << curformdat->getData(); sql << "' where id='" << id <<"'; ";
            }else if(strcmp(curctdisp->getName(),"meta")==0){
                sql<< "update static_content set meta='"; sql << curformdat->getData(); sql << "' where id='" << id <<"'; ";
            }else if(strcmp(curctdisp->getName(),"text")==0){
                sql<< "update static_content set text='"; sql << curformdat->getData(); sql << "' where id='" << id <<"': ";
            }
        }

        if(!sql.empty()){
            Args->database->exec(&sql,res);
            sql.clear();
        }

    }catch(...){};

    sql << "select id,url,meta,text from static_content where id='"; sql << id << "' LIMIT 1";

    if(Args->database->exec(&sql,res)<1){
        return;
    }

    setdiv << "<div id=\"staticsettings\">"
           << "<span>Statische Seiten</span>"
           << "<span>Seite Editieren</span>"
           << "<form method=\"post\" enctype=\"multipart/form-data\">"
           << "<span>Url:</span><br>"
           << "<input name=\"url\" type=\"text\" value=\"" << res[0][1] << "\" /><br>"
           << "<span>Meta:</span><br>"
           << "<textarea name=\"meta\" style=\"width:95%; height:200px;\">"<< res[0][2] <<"</textarea><br>"
           << "<span>Content:</span><br>"
           << "<textarea name=\"text\" style=\"width:95%; min-height:800px;\">"<< res[0][3] <<"</textarea><br>"
           << "<button type=\"submit\" formaction=\"" << Args->config->buildurl("settings/staticpage",url,512) << "\">Back</button>"
           << "<input type=\"submit\" value=\"Save\">"
           << "</form></div>";
}

void blogi::StaticPage::uploadPage(libhttppp::HttpRequest* req, libhtmlpp::HtmlString& setdiv){
}



bool blogi::StaticPage::Controller(netplus::con *curcon,libhttppp::HttpRequest *req){
    char url[512];
    if (strncmp(req->getRequestURL(),Args->config->buildurl("staticpage",url,512),strlen(Args->config->buildurl("staticpage",url,512)))==0){
        std::string surl=req->getRequestURL()+strlen(Args->config->buildurl("staticpage/",url,512));

        if(strstr(req->getRequestURL(),"robots.txt")){
               const char *robot = "User-agent: Googlebot\r\n\r\nUser-agent: *\r\nAllow: /";
               libhttppp::HttpResponse resp;
               resp.setVersion(HTTPVERSION(1.1));
               resp.setState(HTTP200);
               resp.setContentType("text/plain");
               resp.send(curcon,robot,strlen(robot));
               return true;
        }
        libhtmlpp::HtmlString condat;
        libhttppp::HTTPException excep;

        blogi::SQL sql;
        blogi::DBResult res;

        sql << "select url,text,meta from static_content where url='" << surl.c_str() << "' LIMIT 1";

        if (Args->database->exec(&sql,res)<1) {
            excep[libhttppp::HTTPException::Error] << "Staticpage with this url not found!";
            throw excep;
        }

        std::string out;
        condat << "<div id=\"content\" class=\"staticpage\" >"
               << res[0][1]
               << "</div>";
        std::string sid;
        Args->theme->RenderSite(out,req->getRequestURL(),condat,Args->auth->isLoggedIn(req,sid),res[0][2]);
        libhttppp::HttpResponse resp;
        resp.setVersion(HTTPVERSION(1.1));
        resp.setState(HTTP200);
        resp.setContentType("text/html");
        resp.send(curcon,out.c_str(),out.length());
        return true;
    }
    return false;
}

const char * blogi::StaticPage::getAuthor(){
    return "Jan Koester";
}

const char * blogi::StaticPage::getName(){
    return "staticpage";
}

const char * blogi::StaticPage::getVersion(){
    return "20230809";
}
