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
#include <chrono>
#include <thread>
#include <cstring>
#include <cmath>
#include <string>
#include <vector>

#include "plugin.h"
#include "database.h"
#include "theme.h"
#include "conf.h"

#include <json-c/json.h>
#include <json-c/json_object.h>

#include <httppp/http.h>
#include <httppp/exception.h>

#include <htmlpp/html.h>
#include <htmlpp/exception.h>

#include <netplus/exception.h>

#include <confplus/exception.h>


namespace blogi {

    class NginxFiler : public PluginApi {
    public:
        NginxFiler() {
        }

        const char* getName(){
            return "nginxfiler";
        }

        const char* getVersion(){
            return "0.1";
        }

        const char* getAuthor(){
            return "Jan Koester";
        }

        void initPlugin(){
            try{
                _NHost=Args->config->getValue(Args->config->getKey("/BLOGI/NGINXFILER/HOST"),0);
                _NPort=Args->config->getIntValue(Args->config->getKey("/BLOGI/NGINXFILER/PORT"),0);
                _NPrefix=Args->config->getValue(Args->config->getKey("/BLOGI/NGINXFILER/PREFIX"),0);
                _NSSL=Args->config->getIntValue(Args->config->getKey("/BLOGI/NGINXFILER/SSL"),0);
            }catch(confplus::ConfException &e){
                libhttppp::HTTPException err;
                err[libhttppp::HTTPException::Error] << "NginxFiler init failed: " << e.what();
                throw err;
            }
        }

        void RenderUI(std::string path,struct json_object *jobj,libhtmlpp::HtmlString &out){
            int fcount = json_object_array_length(jobj);
            out<<"<div id=\"nginxfiler\"><ul>";
            for(int i =0; i<fcount; ++i) {
                int ntype=-1;
                std::string name,date;
                json_object_object_foreach(json_object_array_get_idx(jobj,i), key, val) {
                    enum json_type type = json_object_get_type(val);
                    switch (type) {
                        case json_type_string: {
                            if(strcmp(key,"type")==0){
                                if(strcmp(json_object_get_string(val),"file")==0){
                                    ntype=0;
                                }else if(strcmp(json_object_get_string(val),"directory")==0){
                                    ntype=1;
                                }
                            }else if(strcmp(key,"name")==0){
                                name=json_object_get_string(val);
                            }else if(strcmp(key,"mtime")==0){
                                date=json_object_get_string(val);
                            }
                            break;
                        }
                        default: {
                            break;
                        }
                    }
                }
                char url[512];
                if(ntype==0)
                    out << "<li class=\"file\" ><img alt=\"file\" src=\""<< Args->config->buildurl("theme/public/file.webp",url,512) << "\" >"
                        << "<a href=\"http://" << _NHost << ":" << _NPort << "/"
                        << _NPrefix << path << name <<"\" >" << name << "</a></li>";
                else if(ntype==1)
                    out << "<li class=\"dir\" ><img alt=\"folder\" src=\""<< Args->config->buildurl("theme/public/folder.webp",url,512) << "\" >"
                        << "<a href=\""<< Args->config->buildurl("nginxfiler",url,512)
                        << path << name << "\" >" << name << "</a></li>";
            }
            if(path!=_NPrefix){
                size_t nend=path.length();
                bool nstart=false;

                while(nend>0){
                    --nend;
                    if(path[nend]!='/' && !nstart){
                        nstart=true;
                    }
                    if(path[nend]=='/' && nstart){
                        break;
                    }
                }

                if(nend>0){
                    char url[512];
                    std::string back=path.substr(0,nend);
                    out << "<li><a href=\"" << Args->config->buildurl("nginxfiler",url,512)
                        << "/" << back.c_str() << "\">..</a></li>";
                }
            }
            out<<"</ul></div>";
        }

        bool Controller(libhttppp::HttpRequest *req,libhtmlpp::HtmlElement *page){
            char url[512];
            if(strncmp(req->getRequestURL(),Args->config->buildurl("nginxfiler",url,512),strlen(Args->config->buildurl("nginxfiler",url,512)))!=0){
                return false;
            }

            std::string path;

            if(req->getRequestURL()+strlen(Args->config->buildurl("nginxfiler",url,512)))
                path=req->getRequestURL()+strlen(Args->config->buildurl("nginxfiler",url,512));

            path+="/";

            std::shared_ptr<netplus::tcp> srvsock;
            std::shared_ptr<netplus::socket> cltsock;
            try{
                try{
                    srvsock=std::make_shared<netplus::tcp>(_NHost.c_str(),_NPort,1,0);
                    cltsock=std::make_shared<netplus::tcp>();
                    srvsock->setnonblocking();
                    srvsock->connect(cltsock);
                    cltsock->setnonblocking();
                }catch(netplus::NetException &e){
                    libhttppp::HTTPException he;
                    he[libhttppp::HTTPException::Error] << e.what();
                    throw he;
                }

                std::string nurl=_NPrefix+path;

                libhttppp::HttpRequest nreq;
                nreq.setRequestType(GETREQUEST);
                nreq.setRequestURL(nurl.c_str());
                nreq.setRequestVersion(HTTPVERSION(1.1));
                *nreq.setData("connection") << "keep-alive";
                *nreq.setData("host")  << _NHost.c_str() << ":" << _NPort;
                *nreq.setData("accept") << "text/json";
                *nreq.setData("user-agent") << "blogi/1.0 (Alpha Version 0.1)";
                nreq.send(cltsock,srvsock);

                char data[16384];
                int recv,tries=0,chunklen=0;

                try{
                    for(;;){
                        recv=srvsock->recvData(cltsock,data,16384);
                        if(recv>0)
                                break;
                        if(tries>5){
                            netplus::NetException e;
                            e[netplus::NetException::Error] << "nginxfiler: can't reach nginx server !";
                            throw e;
                        }
                        std::this_thread::sleep_for(std::chrono::milliseconds(5*tries));
                        ++tries;
                    }
                }catch(netplus::NetException &e){
                    libhttppp::HTTPException he;
                    he[libhttppp::HTTPException::Error] << e.what();
                    throw he;
                }



                std::string json;
                libhttppp::HttpResponse res;
                size_t hsize=0,cpos;
                bool chunked=false;

                int rlen=0;


                hsize=res.parse(data,recv);

                recv-=hsize;

                memmove(data,data+hsize,recv);

                cpos=0;

                try{
                    if(strcmp(res.getTransferEncoding(),"chunked")==0){
                        chunklen=readchunk(data,recv,cpos);
                        chunked=true;
                    }else{
                        throw;
                    }
                }catch(...){
                    rlen=res.getContentLength();
                    json.resize(rlen);
                }

                tries=0;

                if(!chunked){
                    do{
                        try{
                            json.append(data+cpos,recv);
                            rlen-=recv;
                            if(rlen>0){
                                tries=0;
                                for(;;){
                                    cpos=0;
                                    recv=srvsock->recvData(cltsock,data,16384);
                                    if(recv>0)
                                        break;
                                    if(tries>5){
                                        netplus::NetException e;
                                        e[netplus::NetException::Error] << "nginxfiler: can't reach nginx server !";
                                        throw e;
                                    }
                                    ++tries;;
                                    std::this_thread::sleep_for(std::chrono::milliseconds(100));

                                }
                            }
                        }catch(netplus::NetException &e){
                            libhttppp::HTTPException ee;
                            ee[libhttppp::HTTPException::Error] << e.what();
                            throw ee;
                        }
                    }while(rlen>0);
                }else{

                    size_t readed=0;

                    for(;;){
                        if(recv - cpos > 0){

                            if(readed==chunklen){
                                if( (chunklen=readchunk(data,recv,cpos)) == 0 ){
                                    break;
                                }
                                readed=0;
                            }

                            size_t len = (chunklen - readed) < (recv - cpos) ? (chunklen - readed)  : (recv - cpos);

                            json.append(data+cpos,len);
                            cpos+=len;
                            readed+=len;
                        }else{
                            try{
                                tries=0;
                                for(;;){
                                    recv=srvsock->recvData(cltsock,data,16384);
                                    cpos=0;
                                    if(recv == 0 && tries>10){
                                        netplus::NetException e;
                                        e[netplus::NetException::Error] << "nginxfiler: can't reach nginx server !";
                                        throw e;
                                    }else if(recv !=0 ){
                                        break;
                                    }
                                    ++tries;
                                    std::this_thread::sleep_for(std::chrono::milliseconds(5*tries));

                                }
                            }catch(netplus::NetException &e){
                                libhttppp::HTTPException ee;
                                ee[libhttppp::HTTPException::Error] << e.what();
                                throw ee;
                            }
                        }
                    };
                }

                struct json_object *ndir;
                ndir = json_tokener_parse(json.c_str());

                if(!ndir){
                    libhttppp::HTTPException e;
                    e[libhttppp::HTTPException::Error] << "nginxfiler: counld't read json !";
                    throw e;
                }

                libhtmlpp::HtmlString fileHtml,out;
                std::string sid;

                try{
                    RenderUI(path,ndir,fileHtml);

                    libhtmlpp::HtmlElement *fel=fileHtml.parse();

                    if(page->getElementbyID("main") && fel)
                        page->getElementbyID("main")->appendChild(fel);

                }catch(libhtmlpp::HTMLException &e){
                    libhttppp::HTTPException ee;
                    ee[libhttppp::HTTPException::Error] << e.what();
                    throw ee;
                }

                Args->theme->printSite(out,page,req->getRequestURL(),Args->auth->isLoggedIn(req,sid));

                libhttppp::HttpResponse curres;
                curres.setVersion(HTTPVERSION(1.1));
                curres.setContentType("text/html");
                curres.setState(HTTP200);
                curres.send(req, out.c_str(),out.size());
            }catch(libhttppp::HTTPException &e){
                libhttppp::HttpResponse curres;
                curres.setVersion(HTTPVERSION(1.1));
                curres.setContentType("text/html");
                curres.setState(HTTP500);
                curres.send(req, e.what(),strlen(e.what()));
            }
            return true;
        }
    private:
        int readchunk(const char *data,size_t datasize,size_t &pos){
            int start=pos;

            char value[512];

            while( (pos < datasize) && data[++pos]!='\r'){};

            int len=pos-start;

            if(len > 512){
                libhttppp::HTTPException ee;
                ee[libhttppp::HTTPException::Error] << "nginxfiler: chunck size: " << len << " to big aborting !";
                throw ee;
            }

            memcpy(value,data+start,len);

            value[len]='\0';

            if (len < 1) {
                return 0;
            }

            int result=strtol(value, NULL, 16);

            pos+=2;

            return result;
        }

        std::string _NHost;
        int         _NPort;
        std::string _NPrefix;
        bool        _NSSL;
    };
};

extern "C" blogi::PluginApi* create() {
    return new blogi::NginxFiler();
}

extern "C" void destroy(blogi::PluginApi* p) {
    delete p;
}

