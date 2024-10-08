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
#include <algorithm>

#include <htmlpp/html.h>

#include <conf.h>

#include "static.h"
#include "conf.h"
#include "editor.h"

blogi::StaticPage::StaticPage(){
}

blogi::StaticPage::~StaticPage(){
}

void blogi::StaticPage::initPlugin(){
    blogi::SQL sql;
    blogi::DBResult res;
    sql << "CREATE TABLE IF NOT EXISTS static_content ("
        << "id integer PRIMARY KEY " << Args->database[0]->autoincrement() << ","
        << "text text,"
        << "meta text,"
        << "url character varying(255) NOT NULL UNIQUE"
        << ");";
    Args->database[0]->exec(&sql,res);
}

bool blogi::StaticPage::haveSettings(){
    return true;
}

void blogi::StaticPage::Settings(const int tid,libhttppp::HttpRequest* req, libhtmlpp::HtmlString& setdiv){
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
            newPage(tid,req,setdiv);
            return;
        }else if(surl=="delpage"){
            delPage(tid,req,setdiv);
            return;
        }else if(surl=="editpage"){
            editPage(tid,req,setdiv);
            return;
        }

    }
SETTINGSINDEX:
    blogi::SQL sql;
    blogi::DBResult res;

    sql << "select id,url from static_content";

    int scount=Args->database[tid]->exec(&sql,res);

    setdiv << "<div id=\"staticsettings\"><span>Statische Seiten</span><table>";
    setdiv << "<tr><th>ID</th><th>URL</th><th>Options</th></tr>";
    for(int i=0; i<scount; ++i){
        setdiv << "<tr><td>" << res[i][0] << "</td><td>"<< Args->config->buildurl("staticpage/",url,512) << res[i][1]
               << "</td>" << "<td><a href=\"" << Args->config->buildurl("settings/staticpage/editpage?pageid=",url,512) << res[i][0] << "\">Edit</a></td>"
               << "<td><a href=\"" << Args->config->buildurl("settings/staticpage/delpage?pageid=",url,512) << res[i][0] << "\">Remove</a></td>" <<"</tr>";
    }
    setdiv << "</table><a href=\"" << Args->config->buildurl("settings/staticpage/newpage",url,512) << "\" >Neue Seite Erstellen</a></div>";
}

void blogi::StaticPage::newPage(const int tid,libhttppp::HttpRequest* req, libhtmlpp::HtmlString& setdiv){
    char url[512];
    blogi::SQL sql;
    blogi::DBResult res;

    libhttppp::HttpForm form;
    form.parse(req);
    std::string surl,meta,err;
    libhtmlpp::HtmlString text;

    try {

        for (libhttppp::HttpForm::MultipartForm::Data* curformdat = form.MultipartFormData.getFormData(); curformdat; curformdat = curformdat->nextData()) {

            for(libhttppp::HttpForm::MultipartForm::Data::ContentDisposition* curctdisp = curformdat->getDisposition();
                curctdisp; curctdisp=curctdisp->nextContentDisposition()){

                std::string data;
                libhtmlpp::HtmlString result;

                std::copy(curformdat->Value.begin(),curformdat->Value.end(),
                      std::inserter<std::string>(data,data.begin()));

                if(strcmp(curctdisp->getValue(),"url")==0){
                    libhtmlpp::HtmlEncode(data.c_str(),&result);
                    surl=result.c_str();
                }else if(strcmp(curctdisp->getValue(),"meta")==0){
                    libhtmlpp::HtmlEncode(data.c_str(),&result);
                    meta=result.c_str();
                }else if(strcmp(curctdisp->getValue(),"text")==0){
                    text=&data;
                }
            }
        }
    }catch(...){};

    if(!surl.empty() && ( !text.empty() && text.validate(&err) ) ){
        sql << "INSERT INTO static_content (url,meta,text) VALUES('";
        sql.escaped(surl.c_str()) << "','";
        sql.escaped(meta.c_str()) << "','";
        sql.escaped(text.c_str()) <<"');";
        Args->database[tid]->exec(&sql,res);
        sql.clear();
        setdiv << "<div id=\"staticsettings\"><span>Added succesfully! </span></div>";
    }else{
        setdiv << "<div id=\"staticsettings\">";
        if(!err.empty()){
           setdiv<<"<span style=\"color:red;\" >Invalid Html: " << err << "</span>";
        }
        setdiv << "<span>Statische Seiten</span>"
        << "<span>Neue Seite</span>"
        << "<form method=\"post\" enctype=\"multipart/form-data\">"
        << "<span>Url:</span><br>";
        if(surl.empty())
            setdiv << "<input name=\"url\" type=\"text\" style=\"color:red;\" value=\"newurl\" /><br>";
        else
            setdiv << "<input name=\"url\" type=\"text\" value=\""<< surl <<"\" /><br>";
        setdiv << "<span>Meta:</span><br>"
        << "<textarea name=\"meta\" style=\"width:95%; height:200px;\"> Schlagwörter für Google </textarea><br>"
        << "<span>Content:</span><br>";
        Args->edit->displayEditor("text",text.c_str(),setdiv);
        setdiv<< "<button type=\"submit\" formaction=\"" << Args->config->buildurl("settings/staticpage",url,512) << "\">Back</button>"
        << "<input type=\"submit\" value=\"Save\">"
        << "</form></div>";
    }
}

void blogi::StaticPage::delPage(const int tid,libhttppp::HttpRequest* req, libhtmlpp::HtmlString& setdiv){
    char url[512];
    int id=-1;
    blogi::SQL sql;
    blogi::DBResult res;
    bool confirmed=false;

    libhttppp::HttpForm form;
    form.parse(req);

    for(libhttppp::HttpForm::UrlcodedForm::Data *cdat=form.UrlFormData.getFormData(); cdat; cdat=cdat->nextData()){
        if(strcmp(cdat->getKey(),"pageid")==0){
            size_t pgidlen=strlen(cdat->getValue());
            for(size_t i =0; i<pgidlen; ++i){
                if(!isdigit(cdat->getValue()[i])){
                    libhttppp::HTTPException excep;
                    excep[libhttppp::HTTPException::Error] << "Wrong formted Pageid!";
                    throw excep;
                }
            }
            id=atoi(cdat->getValue());
        }else if(strcmp(cdat->getKey(),"confirmed")==0){
            if(strcmp(cdat->getValue(),"true")==0)
                confirmed=true;
        }
    }

    if(confirmed){
        sql << "DELETE FROM static_content WHERE id='" << id << "'";
        Args->database[tid]->exec(&sql,res);
        setdiv << "<div id=\"staticsettings\"><span>page with id " << id << " is removed !</span></div>";
    }else{
        setdiv << "<div id=\"staticsettings\"><span>You wan't remove the page with id " << id << "?</span><br>"
               << "<a href=\"" << Args->config->buildurl("settings/staticpage/delpage?",url,512) << "pageid=" << id << "&confirmed=true\">Yes</a><a href=\"" << Args->config->buildurl("settings/staticpage",url,512) << "\"> NO </a>"
               << "</div>";
    }
}

void blogi::StaticPage::editPage(const int tid,libhttppp::HttpRequest* req, libhtmlpp::HtmlString& setdiv){
    char url[512];
    int id=-1;
    blogi::SQL sql;
    blogi::DBResult res;

    libhttppp::HttpForm form;
    form.parse(req);

    try {
        sql.clear();

        for (libhttppp::HttpForm::MultipartForm::Data* curformdat = form.MultipartFormData.getFormData(); curformdat; curformdat = curformdat->nextData()) {
            libhttppp::HttpForm::MultipartForm::Data::ContentDisposition* curctdisp = curformdat->getDisposition();{
                if(strcmp(curctdisp->getValue(),"pageid")==0){
                    for(size_t i =0; i<curformdat->Value.size(); ++i){
                        if(!isdigit(curformdat->Value[i])){
                            libhttppp::HTTPException excep;
                            excep[libhttppp::HTTPException::Error] << "Wrong formted Pageid!";
                            throw excep;
                        }
                    }
                    std::vector<char> buf;
                    std::copy(curformdat->Value.begin(),curformdat->Value.end(),
                              std::inserter<std::vector<char>>(buf,buf.begin()));
                    buf.push_back('\0');
                    id=atoi(buf.data());
                }
            }
        }


        if(id>=0){
            for (libhttppp::HttpForm::MultipartForm::Data* curformdat = form.MultipartFormData.getFormData(); curformdat; curformdat = curformdat->nextData()) {
                for(libhttppp::HttpForm::MultipartForm::Data::ContentDisposition* curctdisp = curformdat->getDisposition(); curctdisp; curctdisp=curctdisp->nextContentDisposition()){

                    std::vector<char> data;
                    libhtmlpp::HtmlString result;

                    std::copy(curformdat->Value.begin(),curformdat->Value.end(),
                              std::inserter<std::vector<char>>(data,data.begin()));

                    data.push_back('\0');

                    if(strcmp(curctdisp->getValue(),"url")==0){
                        libhtmlpp::HtmlEncode(data.data(),&result);
                        sql<< "update static_content set url='" << result.c_str() << "' where id='" << id <<"'; ";
                    }else if(strcmp(curctdisp->getValue(),"meta")==0){
                        libhtmlpp::HtmlEncode(data.data(),&result);
                        sql<< "update static_content set meta='" << result.c_str() << "' where id='" << id <<"'; ";
                    }else if(strcmp(curctdisp->getValue(),"text")==0){
                        sql<< "update static_content set text='"; sql.escaped(data.data()) << "' where id='" << id <<"'; ";
                    }
                }
            }
            if(!sql.empty()){
                Args->database[tid]->exec(&sql,res);
                sql.clear();
            }
        }
    }catch(...){};

    if(id<0){

        for(libhttppp::HttpForm::UrlcodedForm::Data *cdat=form.UrlFormData.getFormData(); cdat; cdat=cdat->nextData()){
            if(strcmp(cdat->getKey(),"pageid")==0){
                size_t pgidlen=strlen(cdat->getValue());
                for(size_t i =0; i<pgidlen; ++i){
                        if(!isdigit(cdat->getValue()[i])){
                            libhttppp::HTTPException excep;
                            excep[libhttppp::HTTPException::Error] << "Wrong formted Pageid!";
                            throw excep;
                        }
                }
                id=atoi(cdat->getValue());
            }
        }

        if(id<0){
            libhttppp::HTTPException excep;
            excep[libhttppp::HTTPException::Error] << "Staticpage with this id not found!";
            throw excep;
        }
    }

    sql << "select id,url,meta,text from static_content where id='"; sql << id << "' LIMIT 1";

    if(Args->database[tid]->exec(&sql,res)<1){
        return;
    }

    setdiv << "<div id=\"staticsettings\">"
    << "<span>Statische Seiten</span>"
    << "<span>Seite Editieren</span>"
    << "<form method=\"post\" enctype=\"multipart/form-data\">"
    << "<input name=\"pageid\" type=\"text\" value=\"" << res[0][0] << "\" style=\"display:none;\" />"
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

void blogi::StaticPage::uploadPage(const int tid,libhttppp::HttpRequest* req, libhtmlpp::HtmlString& setdiv){
}



bool blogi::StaticPage::Controller(const int tid,libhttppp::HttpRequest *req,libhtmlpp::HtmlElement *page){
    char url[512];
    if (strncmp(req->getRequestURL(),Args->config->buildurl("staticpage",url,512),strlen(Args->config->buildurl("staticpage",url,512)))==0){
        std::string surl=req->getRequestURL()+strlen(Args->config->buildurl("staticpage/",url,512));

        if(strstr(req->getRequestURL(),"robots.txt")){
               const char *robot = "User-agent: Googlebot\r\n\r\nUser-agent: *\r\nAllow: /";
               libhttppp::HttpResponse resp;
               resp.setVersion(HTTPVERSION(1.1));
               resp.setState(HTTP200);
               resp.setContentType("text/plain");
               resp.send(req,robot,strlen(robot));
               return true;
        }
        libhtmlpp::HtmlString condat;
        libhttppp::HTTPException excep;

        blogi::SQL sql;
        blogi::DBResult res;

        sql << "select url,text,meta from static_content where url='"; sql.escaped(surl.c_str()) << "' LIMIT 1";

        if (Args->database[tid]->exec(&sql,res)<1) {
            excep[libhttppp::HTTPException::Error] << "Staticpage with this url not found!";
            throw excep;
        }

        libhtmlpp::HtmlString out;
        condat << "<div id=\"content\" class=\"staticpage\" >"
               << res[0][1]
               << "</div>";
        std::string sid;
        page->getElementbyID("main")->insertChild(condat.parse());;

        Args->theme->printSite(tid,out,page,req->getRequestURL(),Args->auth->isLoggedIn(tid,req,sid),res[0][2]);
        libhttppp::HttpResponse resp;
        resp.setVersion(HTTPVERSION(1.1));
        resp.setState(HTTP200);
        resp.setContentType("text/html");
        resp.send(req,out.c_str(),out.size());
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
