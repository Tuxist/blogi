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


bool blogi::StaticPage::Controller(netplus::con *curcon,libhttppp::HttpRequest *req){
    char url[512];
    if (strncmp(req->getRequestURL(),Args->config->buildurl("staticpage",url,512),strlen(Args->config->buildurl("staticpage",url,512)))==0){
        std::string surl=req->getRequestURL()+strlen(Args->config->buildurl("staticpage/",url,512));
	if(surl.rfind('?')>0){
            surl=surl.substr(0,surl.rfind('?'));
	}
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
