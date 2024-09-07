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
#include <fstream>

#include <signal.h>
#include <string.h>
#include <errno.h>
#include <thread>

#include <netplus/exception.h>

#include <httppp/exception.h>
#include <httppp/http.h>
#include <httppp/httpd.h>

#include <htmlpp/html.h>
#include <htmlpp/exception.h>

#include <cmdplus/cmdplus.h>

#ifndef Windows
#include <unistd.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#endif

#include "session.h"
#include "auth.h"
#include "database.h"
#include "conf.h"

#include "blogi.h"

#include "pgsql.cpp"
#include "sqlite.cpp"


blogi::Blogi::Blogi(Config *blgcfg,netplus::socket *serversocket) : HttpEvent(serversocket){

    PlgArgs = new PluginArgs;
    PlgArgs->config=blgcfg;

    PlgArgs->database=new Database*[threads];
    for(int i=0; i<threads; ++i){
        if(strcmp(PlgArgs->config->getdbdriver(),"pgsql")==0)
            PlgArgs->database[i]= new Postgresql(PlgArgs->config->getdbopts());
        else if(strcmp(PlgArgs->config->getdbdriver(),"sqlite")==0)
            PlgArgs->database[i]= new SQLite(PlgArgs->config->getdbopts());
    }
    PlgArgs->session= new Session();
    PlgArgs->auth=new Auth(PlgArgs->database,PlgArgs->session,PlgArgs->config);
    PlgArgs->edit=new Editor(PlgArgs->config);

    TemplateConfig tplcfg;
    tplcfg.config=blgcfg;
    tplcfg.Theme=tplcfg.config->gettemplate();
    tplcfg.TDatabase=PlgArgs->database;

    PlgArgs->theme=new Template(tplcfg);

    Page = new libhtmlpp::HtmlPage;
    PlgArgs->theme->renderPage(0,"index.html",Page,&Index);
    MPage = new libhtmlpp::HtmlPage;
    PlgArgs->theme->renderPage(0,"mobile.html",MPage,&MIndex);

    PlgArgs->maxthreads=threads;

    BlogiPlg = new Plugin();

    for(int i=0; i<PlgArgs->config->getplgdirs(); ++i){
         BlogiPlg->loadPlugins(PlgArgs->config->getplgdir(i),PlgArgs);
    }
}

blogi::Blogi::~Blogi(){
    delete PlgArgs->edit;
    delete PlgArgs->auth;
    delete PlgArgs->session;
     for(int i=0; i<threads; ++i){
        delete PlgArgs->database[i];
    }
    delete[] PlgArgs->database;
    delete BlogiPlg;
    delete PlgArgs;
    delete Page;
    delete MPage;
}

void blogi::Blogi::loginPage(libhttppp::HttpRequest *curreq,const int tid){
    char url[512];
    libhttppp::HTTPException excep;
    std::string sessid;
    if(PlgArgs->auth->isLoggedIn(tid,curreq,sessid)){
        libhttppp::HTTPException err;
        err[libhttppp::HTTPException::Error] << "you already authenticated please logoff before you login again!";
        throw err;
    }

    libhttppp::HttpForm curform;
    curform.parse(curreq);

    std::string username;
    std::string password;


    for (libhttppp::HttpForm::UrlcodedForm::Data* cururlform = curform.UrlFormData.getFormData(); cururlform;
        cururlform = cururlform->nextData()) {
        std::cout << cururlform->getKey() <<std::endl;
        if (strcmp(cururlform->getKey(), "username") == 0) {
            username = cururlform->getValue();
        }else if (strcmp(cururlform->getKey(), "password") == 0) {
            password = cururlform->getValue();
        }
    }


    libhtmlpp::HtmlString out;

    std::shared_ptr<libhtmlpp::HtmlElement> index;

    if(curreq->isMobile())
        *index=MIndex;
    else
        *index=Index;

    if (username.empty() || password.empty()) {
        libhtmlpp::HtmlString condat;
        condat << "<div id=\"content\">"
               << "<span>Login</span>"
               << "<form action=\""<< PlgArgs->config->buildurl("login",url,512) << "\" method=\"post\">"
               << "username:<br> <input type=\"text\" name=\"username\" value=\"\"><br>"
               << "password:<br> <input type=\"password\" name=\"password\" value=\"\"><br>"
               << "<button type=\"submit\">Submit</button>"
               << "</form>"
               << "</div>";

        if(index->getElementbyID("main"))
            index->getElementbyID("main")->insertChild(condat.parse());

        for(blogi::Plugin::PluginData *curplg=BlogiPlg->getFirstPlugin(); curplg; curplg=curplg->getNextPlg()){
            curplg->getInstace()->Rendering(tid,curreq,&*index);
        }

        PlgArgs->theme->printSite(tid,out,&*index,curreq->getRequestURL(),false);
        libhttppp::HttpResponse curres;
        curres.setState(HTTP200);
        curres.setVersion(HTTPVERSION(1.1));
        curres.setContentType("text/html");
        curres.send(curreq,out.c_str(),out.size());
        return;
    }

    std::string sid;

    if(PlgArgs->auth->login(tid,username.c_str(),password.c_str(),sid)){
        const char *sessid = PlgArgs->session->createSession(sid.c_str());
        PlgArgs->session->addSessionData(sessid,"sid",sid.c_str(),sid.length());
        PlgArgs->session->addSessionData(sessid,"username",username.c_str(), username.length());
        libhttppp::HttpResponse curres;
        libhttppp::HttpCookie cookie;
        cookie.setcookie(&curres, "sessionid", sessid,nullptr,PlgArgs->config->getDomain(),-1,
                         PlgArgs->config->buildurl("",url,512),false,"1","Lax");
        curres.setState(HTTP307);
        curres.setVersion(HTTPVERSION(1.1));
        *curres.setData("Location") << PlgArgs->config->getstartpage();
        curres.setContentType("text/html");
        curres.send(curreq, nullptr, 0);
    }else{
        libhttppp::HttpResponse curres;
        libhttppp::HttpCookie cookie;
        curres.setState(HTTP403);
        curres.setVersion(HTTPVERSION(1.1));
        curres.setContentType("text/html");

        libhtmlpp::HtmlString condat;
        condat << "<div id=\"content\">"
               << "<span>Reason: Wrong Username or Password !</span>"
               << "<span>Login</span>"
               << "<form action=\""<< PlgArgs->config->buildurl("login",url,512) << "\" method=\"post\">"
               << "username:<br> <input type=\"text\" name=\"username\" value=\"\"><br>"
               << "password:<br> <input type=\"password\" name=\"password\" value=\"\"><br>"
               << "<button type=\"submit\">Submit</button>"
               << "</form>"
               << "</div>";

        curres.send(curreq, condat.c_str(), condat.size());
    }
}

void blogi::Blogi::logoutPage(libhttppp::HttpRequest *curreq,const int tid){
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
    curres.send(curreq,nullptr,0);
}

void blogi::Blogi::settingsPage(libhttppp::HttpRequest* curreq,const int tid){
    libhttppp::HttpCookie cookie;
    cookie.parse(curreq);
    std::string sessid;

    if(!PlgArgs->auth->isLoggedIn(tid,curreq,sessid)){
        libhttppp::HTTPException err;
        err[libhttppp::HTTPException::Error] << "you are not logged in permission denied";
        throw err;
    }

    libhtmlpp::HtmlString out;
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
                curplg->getInstace()->Settings(tid,curreq,setgui);
        }
    }

    setgui << "</td></tr></table></div>";

    libhtmlpp::HtmlElement *index;
    if(curreq->isMobile())
        index= new libhtmlpp::HtmlElement(MIndex);
    else
        index= new libhtmlpp::HtmlElement(Index);

    index->getElementbyID("main")->appendChild(setgui.parse());

    for(blogi::Plugin::PluginData *curplg=BlogiPlg->getFirstPlugin(); curplg; curplg=curplg->getNextPlg()){
        curplg->getInstace()->Rendering(tid,curreq,index);
    }

    PlgArgs->theme->printSite(tid,out,index,curreq->getRequestURL(),false);
    libhttppp::HttpResponse curres;
    curres.setState(HTTP200);
    curres.setVersion(HTTPVERSION(1.1));
    curres.setContentType("text/html");
    curres.send(curreq,out.c_str(),out.size());
    delete index;
}


void blogi::Blogi::RequestEvent(libhttppp::HttpRequest *curreq,const int tid,void *args){
    char url[512];
    try{;
        libhttppp::HttpHeader::HeaderData *hip=curreq->getData("x-real-ip");
        libhttppp::HttpHeader::HeaderData  *useragent=curreq->getData("user-agent");

        if(hip)
            std::cout <<"Request from: " << curreq->getData(hip) << " url: " << curreq->getRequestURL();
        if(useragent)
            std::cout << " agent: " << curreq->getData(useragent);

        std::cout << std::endl;

RETRY_REQUEST:
        try{
            /*blogi internal pages and redirections*/
            if(strcmp(curreq->getRequestURL(),"/")==0 || strcmp(curreq->getRequestURL(),PlgArgs->config->getprefix())==0){
                libhttppp::HttpResponse curres;
                curres.setState(HTTP307);
                curres.setVersion(HTTPVERSION(1.1));
                *curres.setData("Location") << PlgArgs->config->buildurl("content/tag",url,512);
                curres.setContentType("text/html");
                curres.send(curreq, nullptr, 0);
                return;
            }else if(strncmp(curreq->getRequestURL(),PlgArgs->config->buildurl("logout",url,512),strlen(PlgArgs->config->buildurl("logout",url,512)))==0){
                logoutPage(curreq,tid);
                return;
            }else if(strncmp(curreq->getRequestURL(),PlgArgs->config->buildurl("login",url,512),strlen(PlgArgs->config->buildurl("login",url,512)))==0){
                loginPage(curreq,tid);
                return;
            }else if(strncmp(curreq->getRequestURL(),PlgArgs->config->buildurl("settings",url,512),strlen(PlgArgs->config->buildurl("settings",url,512)))==0){
                settingsPage(curreq,tid);
                return;
            }else if(strncmp(curreq->getRequestURL(),PlgArgs->config->buildurl("editor",url,512),strlen(PlgArgs->config->buildurl("editor",url,512)))==0){
                PlgArgs->edit->Controller(curreq);
                return;
            }else if (strstr(curreq->getRequestURL(),"robots.txt")){
                const char *robot = "user-agent: *\r\ndisallow: /blog/settings/";
                libhttppp::HttpResponse resp;
                resp.setVersion(HTTPVERSION(1.1));
                resp.setState(HTTP200);
                resp.setContentType("text/plain");
                resp.send(curreq,robot,strlen(robot));
                return;
            }

            if(!PlgArgs->theme->Controller(tid,curreq)){
                libhtmlpp::HtmlElement *index;
                if(curreq->isMobile())
                    index= new libhtmlpp::HtmlElement(MIndex);
                else
                    index= new libhtmlpp::HtmlElement(Index);;

                for(blogi::Plugin::PluginData *curplg=BlogiPlg->getFirstPlugin(); curplg; curplg=curplg->getNextPlg()){
                    curplg->getInstace()->Rendering(tid,curreq,index);
                }

                for(blogi::Plugin::PluginData *curplg=BlogiPlg->getFirstPlugin(); curplg; curplg=curplg->getNextPlg()){
                    PluginApi *api=curplg->getInstace();
                    std::string url=PlgArgs->config->getprefix();
                    url+="/";
                    url+=api->getName();
                    if(strncmp(curreq->getRequestURL(),url.c_str(),url.length())==0){
                        if(api->Controller(tid,curreq,index)){
                            delete index;
                            return;
                        }
                    }
                }
                delete index;

                libhtmlpp::HtmlString output;
                libhtmlpp::HtmlString err;
                err << "<!DOCTYPE html><html><body style=\"color:rgb(238, 238, 238); background:rgb(35, 38, 39);\"><span>"
                << "Seite oder Inhalt nicht gefudnen"
                << "</span><br/><a style=\"text-decoration: none; color: rgb(58,212, 58);\" href=\""
                <<  PlgArgs->config->getstartpage()
                << "\" >Zur&uuml;ck zur Startseite</a></body></html>";
                libhtmlpp::print(err.parse(),output);
                libhttppp::HttpResponse resp;
                resp.setVersion(HTTPVERSION(1.1));
                resp.setState(HTTP404);
                resp.setContentType("text/html");
                resp.send(curreq,output.c_str(),output.size());
            }
        }catch(libhttppp::HTTPException &e){
            if(!PlgArgs->database[tid]->isConnected()){
                PlgArgs->database[tid]->reset();
                goto RETRY_REQUEST;
            }
            throw e;
        }
    }catch(libhttppp::HTTPException &e){
        if(e.getErrorType() == libhttppp::HTTPException::Note || e.getErrorType() == libhttppp::HTTPException::Warning)
            return;
        libhtmlpp::HtmlString output;
        libhtmlpp::HtmlString err,hreason;
        libhtmlpp::HtmlEncode(e.what(),&hreason);
        err << "<!DOCTYPE html><html><body style=\"color:rgb(238, 238, 238); background:rgb(35, 38, 39);\"><span>"
        << hreason
        << "</span><br/><a style=\"text-decoration: none; color: rgb(58,212, 58);\" href=\""
        <<  PlgArgs->config->getstartpage()
        << "\" >Zur&uuml;ck zur Startseite</a></body></html>";
        libhtmlpp::print(err.parse(),output);
        libhttppp::HttpResponse resp;
        resp.setVersion(HTTPVERSION(1.1));
        resp.setState(HTTP500);
        resp.setContentType("text/html");
        resp.send(curreq,output.c_str(),output.size());
    }
}

void blogi::Blogi::ResponseEvent(libhttppp::HttpRequest* curreq,const int tid,void *args){
    if(PlgArgs->theme->Response(tid,curreq))
        return;

    for(blogi::Plugin::PluginData *curplg=BlogiPlg->getFirstPlugin(); curplg; curplg=curplg->getNextPlg()){
        PluginApi *api=curplg->getInstace();
        std::string url=PlgArgs->config->getprefix();
        url+="/";
        url+=api->getName();
        if(strncmp(curreq->getRequestURL(),url.c_str(),url.length())==0){
            if(api->Response(tid,curreq)){
                return;
            }
        }
    }
}


class HttpConD : public libhttppp::HttpD {
public:
    HttpConD(blogi::Config *blgcfg)
            : HttpD(blgcfg->gethttpaddr(),blgcfg->gethttpport(),blgcfg->gethttpmaxcon(),blgcfg->getsslcertpath(),blgcfg->getsslkeypath()){
        libhttppp::HTTPException httpexception;
        try {
            blogi::Session session;
            try{
                blogi::Blogi blg(blgcfg,getServerSocket());
#ifndef Windows
                struct passwd *pwd=getpwnam("blogi");
                seteuid(pwd->pw_uid);
                setegid(pwd->pw_gid);
#endif
                blg.runEventloop();
            }catch(libhttppp::HTTPException &e){
                std::cerr << e.what() << std::endl;
            }
        }catch(netplus::NetException &e){
            std::cerr << e.what() << std::endl;
        }
    };
};

void logFiles(const char *path,int fd){
    int pfd=open(path,O_APPEND|O_CREAT|O_WRONLY,0x760);

    struct passwd *pwd=getpwnam("blogi");

    fchown(pfd,pwd->pw_uid,pwd->pw_gid);

    dup2(pfd, fd);
    close(pfd);
}

int main(int argc, char** argv){
    cmdplus::CmdController *BlogiCmdCtl;
    BlogiCmdCtl = &cmdplus::CmdController::getInstance();

    BlogiCmdCtl->registerCmd("help", 'h', false, (const char*) nullptr, "Helpmenu");

    if (BlogiCmdCtl->getCmdbyKey("help") && BlogiCmdCtl->getCmdbyKey("help")->getFound()) {
        BlogiCmdCtl->printHelp();
        return 0;
    }

    BlogiCmdCtl->registerCmd("config",'c', true,(const char*) nullptr,"Blogi Config File");
    BlogiCmdCtl->registerCmd("debug",'d', false,(int)0,"Blogi runs Debugmod");

    BlogiCmdCtl->parseCmd(argc,argv);

    const char *config = BlogiCmdCtl->getCmdbyKey("config")->getValue();
    int debug = BlogiCmdCtl->getCmdbyKey("debug")->getValueInt();

    blogi::Config *cins;


    if(config)
        cins = new blogi::Config(config);
    else
        return -1;

    if(!cins)
        return -1;

    try{
#ifndef Windows
        signal(SIGPIPE, SIG_IGN);

        if(debug==0){
            if(getuid()!=0){
                libhttppp::HTTPException e;
                e[libhttppp::HTTPException::Critical] << "must be run as root!";
                throw e;
            }

            logFiles("/var/log/blogi/access.log",STDOUT_FILENO);
            logFiles("/var/log/blogi/error.log",STDERR_FILENO);

            int pid = daemon(1,1);
            if(pid==0){
                HttpConD blogiD(cins);
            }else if(pid>0){

            }
        }else{
            HttpConD blogiD(cins);
        }
#else
        HttpConD blogiD(cins);
#endif
    }catch(libhttppp::HTTPException &e){
        std::cerr << e.what() << std::endl;
        return -1;
    }
    return 0;
    // delete cins;
}


