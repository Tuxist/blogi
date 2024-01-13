/*******************************************************************************
Copyright (c) 2021, Jan Koester jan.koester@gmx.net
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

#include <iostream>

#include <signal.h>
#include <string.h>
#include <errno.h>

#include <netplus/eventapi.h>
#include <netplus/exception.h>

#include <cryptplus/cryptplus.h>

#include <httppp/exception.h>
#include <httppp/http.h>
#include <httppp/httpd.h>

#include <htmlpp/html.h>
#include <htmlpp/exception.h>

#include <cmdplus/cmdplus.h>

#include "session.h"
#include "auth.h"
#include "database.h"
#include "conf.h"

#include "blogi.h"

#include "pgsql.cpp"
#include "sqlite.cpp"

blogi::Blogi::Blogi(netplus::socket *serversocket) : event(serversocket){

    PlgArgs = new PluginArgs;
    PlgArgs->config=Config::getInstance();
    if(strcmp(PlgArgs->config->getdbdriver(),"pgsql")==0)
        PlgArgs->database= new Postgresql(PlgArgs->config->getdbopts());
    else if(strcmp(PlgArgs->config->getdbdriver(),"sqlite")==0)
        PlgArgs->database= new SQLite(PlgArgs->config->getdbopts());
    PlgArgs->session= new Session();
    PlgArgs->auth=new Auth(PlgArgs->database,PlgArgs->session);
    PlgArgs->edit=new Editor(PlgArgs->config);

    TemplateConfig tplcfg;
    tplcfg.config=Config::getInstance();
    tplcfg.Theme=tplcfg.config->gettemplate();
    tplcfg.TDatabase=PlgArgs->database;

    PlgArgs->theme=new Template(tplcfg);

    PlgArgs->theme->renderPage("index.html",Page,Index);
    PlgArgs->theme->renderPage("mobile.html",MPage,MIndex);

    BlogiPlg = new Plugin();

    for(int i=0; i<PlgArgs->config->getplgdirs(); ++i){
         BlogiPlg->loadPlugins(PlgArgs->config->getplgdir(i),PlgArgs);
    }
}

blogi::Blogi::~Blogi(){
    delete PlgArgs->edit;
    delete PlgArgs->auth;
    delete PlgArgs->session;
    delete PlgArgs->database;
    delete BlogiPlg;
    delete PlgArgs;
}

void blogi::Blogi::loginPage(netplus::con*curcon,libhttppp::HttpRequest *curreq){
    char url[512];
    libhttppp::HTTPException excep;
    std::string sessid;
    if(PlgArgs->auth->isLoggedIn(curreq,sessid)){
        libhttppp::HTTPException err;
        err[libhttppp::HTTPException::Error] << "you already authenticated please logoff before you login again!";
        throw err;
    }

    libhttppp::HttpForm curform;
    curform.parse(curreq);

    const char *username=nullptr;
    const char *password=nullptr;

    if (curform.getUrlcodedFormData()) {
        for (libhttppp::HttpForm::UrlcodedFormData* cururlform = curform.getUrlcodedFormData(); cururlform;
             cururlform = cururlform->nextUrlcodedFormData()) {
            if (strcmp(cururlform->getKey(), "username") == 0) {
                username = cururlform->getValue();
            }
            else if (strcmp(cururlform->getKey(), "password") == 0) {
                password = cururlform->getValue();
            }
             }
    }

    std::string out;

    libhtmlpp::HtmlElement index;
    if(curreq->isMobile())
        index=&MIndex;
    else
        index=&Index;

    if (!username || !password) {
        libhtmlpp::HtmlString condat;
        condat << "<div id=\"content\">"
               << "<span>Login</span>"
               << "<form action=\""<< PlgArgs->config->buildurl("login",url,512) << "\" method=\"post\">"
               << "username:<br> <input type=\"text\" name=\"username\" value=\"\"><br>"
               << "password:<br> <input type=\"password\" name=\"password\" value=\"\"><br>"
               << "<button type=\"submit\">Submit</button>"
               << "</form>"
               << "</div>";

        index.getElementbyID("main")->insertChild(condat.parse());

        for(blogi::Plugin::PluginData *curplg=BlogiPlg->getFirstPlugin(); curplg; curplg=curplg->getNextPlg()){
            curplg->getInstace()->Rendering(curreq,index);
        }

        PlgArgs->theme->printSite(out,index,curreq->getRequestURL(),false);
        libhttppp::HttpResponse curres;
        curres.setState(HTTP200);
        curres.setVersion(HTTPVERSION(1.1));
        curres.setContentType("text/html");
        curres.send(curcon,out.c_str(),out.length());
        return;
    }

    std::string sid;

    if(PlgArgs->auth->login(username,password,sid)){
        const char *sessid = PlgArgs->session->createSession(sid.c_str());
        PlgArgs->session->addSessionData(sessid,"sid",sid.c_str(),sid.length());
        PlgArgs->session->addSessionData(sessid,"username",username, strlen(username));
        libhttppp::HttpResponse curres;
        libhttppp::HttpCookie cookie;
        cookie.setcookie(&curres, "sessionid", sessid);
        curres.setState(HTTP307);
        curres.setVersion(HTTPVERSION(1.1));
        *curres.setData("Location") << PlgArgs->config->getstartpage();
        curres.setContentType("text/html");
        curres.send(curcon, nullptr, 0);
    }else{
        libhttppp::HTTPException err;
        err[libhttppp::HTTPException::Error] << "Login Failed!";
        throw err;
    }
}

void blogi::Blogi::logoutPage(netplus::con *curcon,libhttppp::HttpRequest *curreq){
    const char *host;
    for(libhttppp::HttpHeader::HeaderData *preq = curreq->getfirstHeaderData(); preq; preq=curreq->nextHeaderData(preq)){
        if(strncmp(curreq->getKey(preq),"Host",4)==0)
            host=curreq->getValue(preq);
    }
    libhttppp::HttpResponse curres;
    libhttppp::HttpCookie cookie;
    cookie.setcookie(&curres,"sessionid","empty");
    curres.setState(HTTP307);
    curres.setVersion(HTTPVERSION(1.1));
    *curres.setData("Location") << PlgArgs->config->getstartpage();
    curres.setContentType("text/html");
    curres.setContentLength(0);
    curres.send(curcon,nullptr,0);
}

void blogi::Blogi::settingsPage(netplus::con* curcon, libhttppp::HttpRequest* curreq){
    libhttppp::HttpCookie cookie;
    cookie.parse(curreq);
    std::string sessid;

    if(!PlgArgs->auth->isLoggedIn(curreq,sessid)){
        libhttppp::HTTPException err;
        err[libhttppp::HTTPException::Error] << "you are not logged in permission denied";
        throw err;
    }

    std::string out;
    libhtmlpp::HtmlString setgui;
    char url[512];

    setgui << "<div id=\"settings\"><table><tr><td id=\"setnav\" ><ul>";
    for(blogi::Plugin::PluginData *curplg=BlogiPlg->getFirstPlugin(); curplg; curplg=curplg->getNextPlg()){
        if(curplg->getInstace()->haveSettings())
            setgui << "<li><a href=\"" << PlgArgs->config->buildurl("settings/",url,512) << curplg->getInstace()->getName()  << "\">"  << curplg->getInstace()->getName() << "</a></li>";
    }
    setgui << "</ul></td><td id=\"setcontent\">";

    int relen=strlen(PlgArgs->config->buildurl("settings/",url,512))-strlen(curreq->getRequestURL());
    if( relen < 0){
        for(blogi::Plugin::PluginData *curplg=BlogiPlg->getFirstPlugin(); curplg; curplg=curplg->getNextPlg()){
            if(strncmp(curreq->getRequestURL()+strlen(PlgArgs->config->buildurl("settings/",url,512)), curplg->getInstace()->getName(),
                strlen(curplg->getInstace()->getName()))==0)
                curplg->getInstace()->Settings(curreq,setgui);
        }
    }

    setgui << "</td></tr></table></div>";

    libhtmlpp::HtmlElement index;
    if(curreq->isMobile())
        index=&MIndex;
    else
        index=&Index;

    index.getElementbyID("main")->appendChild(setgui.parse());

    for(blogi::Plugin::PluginData *curplg=BlogiPlg->getFirstPlugin(); curplg; curplg=curplg->getNextPlg()){
        curplg->getInstace()->Rendering(curreq,index);
    }

    PlgArgs->theme->printSite(out,index,curreq->getRequestURL(),false);
    libhttppp::HttpResponse curres;
    curres.setState(HTTP200);
    curres.setVersion(HTTPVERSION(1.1));
    curres.setContentType("text/html");
    curres.send(curcon,out.c_str(),out.length());
}


void blogi::Blogi::RequestEvent(netplus::con *curcon){
    libhttppp::HttpRequest req;
    char url[512];
    try{
        req.parse(curcon);

        libhttppp::HttpHeader::HeaderData *hip=req.getData("x-real-ip");
        libhttppp::HttpHeader::HeaderData  *useragent=req.getData("user-agent");

        if(hip)
            std::cout <<"Request from: " << req.getData(hip) << " url: " << req.getRequestURL();
        if(useragent)
            std::cout << " agent: " << req.getData(useragent);

        std::cout << std::endl;

RETRY_REQUEST:
        try{
            /*blogi internal pages and redirections*/
            if(strcmp(req.getRequestURL(),"/")==0 || strcmp(req.getRequestURL(),PlgArgs->config->getprefix())==0){
                libhttppp::HttpResponse curres;
                curres.setState(HTTP307);
                curres.setVersion(HTTPVERSION(1.1));
                *curres.setData("Location") << PlgArgs->config->buildurl("content/tag",url,512);
                curres.setContentType("text/html");
                curres.send(curcon, nullptr, 0);
                return;
            }else if(strncmp(req.getRequestURL(),PlgArgs->config->buildurl("logout",url,512),strlen(PlgArgs->config->buildurl("logout",url,512)))==0){
                logoutPage(curcon,&req);
                return;
            }else if(strncmp(req.getRequestURL(),PlgArgs->config->buildurl("login",url,512),strlen(PlgArgs->config->buildurl("login",url,512)))==0){
                loginPage(curcon,&req);
                return;
            }else if(strncmp(req.getRequestURL(),PlgArgs->config->buildurl("settings",url,512),strlen(PlgArgs->config->buildurl("settings",url,512)))==0){
                settingsPage(curcon,&req);
                return;
            }else if(strncmp(req.getRequestURL(),PlgArgs->config->buildurl("editor",url,512),strlen(PlgArgs->config->buildurl("editor",url,512)))==0){
                PlgArgs->edit->Controller(curcon,&req);
                return;
            }else if (strstr(req.getRequestURL(),"robots.txt")){
                const char *robot = "user-agent: *\r\ndisallow: /blog/settings/";
                libhttppp::HttpResponse resp;
                resp.setVersion(HTTPVERSION(1.1));
                resp.setState(HTTP200);
                resp.setContentType("text/plain");
                resp.send(curcon,robot,strlen(robot));
                return;
            }

            libhtmlpp::HtmlElement index;
            if(req.isMobile())
                index=&MIndex;
            else
                index=&Index;

            if(!PlgArgs->theme->Controller(curcon,&req)){

                for(blogi::Plugin::PluginData *curplg=BlogiPlg->getFirstPlugin(); curplg; curplg=curplg->getNextPlg()){
                    curplg->getInstace()->Rendering(&req,index);
                }

                for(blogi::Plugin::PluginData *curplg=BlogiPlg->getFirstPlugin(); curplg; curplg=curplg->getNextPlg()){
                    PluginApi *api=curplg->getInstace();
                    std::string url=PlgArgs->config->getprefix();
                    url+="/";
                    url+=api->getName();
                    if(strncmp(req.getRequestURL(),url.c_str(),url.length())==0){
                        if(api->Controller(curcon,&req,index))
                            return;
                    }
                }


                std::string output;
                libhtmlpp::HtmlString err;
                err << "<!DOCTYPE html><html><body style=\"color:rgb(238, 238, 238); background:rgb(35, 38, 39);\"><span>"
                << "Seite oder Inhalt nicht gefudnen"
                << "</span><br/><a style=\"text-decoration: none; color: rgb(58,212, 58);\" href=\""
                <<  PlgArgs->config->getstartpage()
                << "\" >Zur&uuml;ck zur Startseite</a></body></html>";
                libhtmlpp::print(err.parse(),nullptr,output);
                libhttppp::HttpResponse resp;
                resp.setVersion(HTTPVERSION(1.1));
                resp.setState(HTTP404);
                resp.setContentType("text/html");
                resp.send(curcon,output.c_str(),output.length());
            }
        }catch(libhttppp::HTTPException &e){
            if(!PlgArgs->database->isConnected()){
                PlgArgs->database->reset();
                if(PlgArgs->database->isConnected()){
                    goto RETRY_REQUEST;
                }
            }
            throw e;
        }
    }catch(libhttppp::HTTPException &e){
        if(e.getErrorType() == libhttppp::HTTPException::Note || e.getErrorType() == libhttppp::HTTPException::Warning)
            return;
        std::string output;
        libhtmlpp::HtmlString err,hreason;
        libhtmlpp::HtmlEncode(e.what(),hreason);
        err << "<!DOCTYPE html><html><body style=\"color:rgb(238, 238, 238); background:rgb(35, 38, 39);\"><span>"
        << hreason
        << "</span><br/><a style=\"text-decoration: none; color: rgb(58,212, 58);\" href=\""
        <<  PlgArgs->config->getstartpage()
        << "\" >Zur&uuml;ck zur Startseite</a></body></html>";
        libhtmlpp::print(err.parse(),nullptr,output);
        libhttppp::HttpResponse resp;
        resp.setVersion(HTTPVERSION(1.1));
        resp.setState(HTTP500);
        resp.setContentType("text/html");
        resp.send(curcon,output.c_str(),output.length());
    }
}


class HttpConD : public libhttppp::HttpD {
public:
    HttpConD(const char *httpaddr, int port,int maxconnections,const char *sslcertpath,const char *sslkeypath)
            : HttpD(httpaddr,port,maxconnections,sslcertpath,sslkeypath){
        libhttppp::HTTPException httpexception;
        try {
            blogi::Session session;
            try{
                blogi::Blogi blg(getServerSocket());
                blg.runEventloop();
            }catch(libhttppp::HTTPException &e){
                std::cerr << e.what() << std::endl;
            }
        }catch(netplus::NetException &e){
            std::cout << e.what() << std::endl;
        }
    };
private:
};

int main(int argc, char** argv){
    signal(SIGPIPE, SIG_IGN);

    cmdplus::CmdController *BlogiCmdCtl;
    BlogiCmdCtl = &cmdplus::CmdController::getInstance();

    BlogiCmdCtl->registerCmd("help", 'h', false, (const char*) nullptr, "Helpmenu");

    if (BlogiCmdCtl->getCmdbyKey("help") && BlogiCmdCtl->getCmdbyKey("help")->getFound()) {
        BlogiCmdCtl->printHelp();
        return 0;
    }

    BlogiCmdCtl->registerCmd("config",'c', true,(const char*) nullptr,"Blogi Config File");

    BlogiCmdCtl->parseCmd(argc,argv);

    const char *config = BlogiCmdCtl->getCmdbyKey("config")->getValue();

    blogi::Config *cins;
    cins=blogi::Config::getInstance();

    if(config)
        cins->loadconfig(config);
    else
        return -1;

    if(!cins)
        return -1;

    HttpConD(cins->gethttpaddr(),
             cins->gethttpport(),
             cins->gethttpmaxcon(),
             nullptr,
             nullptr
            );
}


