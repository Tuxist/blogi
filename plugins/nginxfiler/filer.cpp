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

#include <netplus/exception.h>

#include <confplus/exception.h>


namespace blogi {
    template<typename T = size_t>
    T Hex2Int(const char* const hexstr, bool* overflow=nullptr)
    {
        if (!hexstr)
            return false;
        if (overflow)
            *overflow = false;

        auto between = [](char val, char c1, char c2) { return val >= c1 && val <= c2; };
        size_t len = strlen(hexstr);
        T result = 0;

        for (size_t i = 0, offset = sizeof(T) << 3; i < len && (int)offset > 0; i++)
        {
            if (between(hexstr[i], '0', '9'))
                result = result << 4 ^ (hexstr[i] - '0');
            else if (between(tolower(hexstr[i]), 'a', 'f'))
                result = result << 4 ^ (tolower(hexstr[i]) - ('a' - 10)); // Remove the decimal part;
            offset -= 4;
        }
        if (((len + ((len % 2) != 0)) << 2) > (sizeof(T) << 3) && overflow)
            *overflow = true;
        return result;
    }

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
            out<<"<div id=\"main\"><ul>";
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
                    out << "<li class=\"file\" ><a href=\"http://" << _NHost << ":" << _NPort << "/"
                        << _NPrefix << path << name <<"\" >" << name << "</a></li>";
                else if(ntype==1)
                    out << "<li class=\"dir\" ><a href=\""<< Args->config->buildurl("nginxfiler",url,512)
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
            out<<"</ul><div>";
        }

        bool Controller(netplus::con *curcon,libhttppp::HttpRequest *req,libhtmlpp::HtmlElement page){
            char url[512];
            if(strncmp(req->getRequestURL(),Args->config->buildurl("nginxfiler",url,512),strlen(Args->config->buildurl("nginxfiler",url,512)))!=0){
                return false;
            }

            std::string path;

            if(req->getRequestURL()+strlen(Args->config->buildurl("nginxfiler",url,512)))
                path=req->getRequestURL()+strlen(Args->config->buildurl("nginxfiler",url,512));

            path+="/";

            netplus::tcp *srvsock=nullptr;
            netplus::socket *cltsock=nullptr;
            try{
                try{
                    srvsock= new netplus::tcp(_NHost.c_str(),_NPort,1,0);
                    srvsock->setnonblocking();
                    cltsock=srvsock->connect();
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
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        ++tries;
                    }
                }catch(netplus::NetException &e){
                    libhttppp::HTTPException he;
                    he[libhttppp::HTTPException::Error] << e.what();
                    throw he;
                }



                std::string json;
                libhttppp::HttpResponse res;
                size_t len=recv,hsize=0,cpos;
                bool chunked=false;

                int rlen=0;


                hsize=res.parse(data,len);

                cpos=hsize-1;

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

                try {
                    if(strcmp(res.getTransferEncoding(),"chunked")==0){
                        chunklen=readchunk(data,recv,hsize);
                        chunked=true;
                    }else{
                        rlen=res.getContentLength();
                        json.resize(rlen);
                    }
                }catch(libhttppp::HTTPException &e){
                    std::cerr << e.what() << std::endl;
                };

                if(!chunked){
                    do{
                        json.append(data+hsize,recv-hsize);
                        rlen-=recv-hsize;
                        if(rlen>0){
                            for(;;){
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
                            hsize=0;
                        }
                    }while(rlen>0);
                }else{
READCHUNK:
                    if(recv >= chunklen){
                        chunklen-=cpos;
                        std::cerr << "recv: " << chunklen << std::endl;
                        json.append(data+cpos,chunklen);
                        cpos+=(chunklen-1);
                        recv-=chunklen;
                    }else{
                        recv-=cpos;
                        std::cerr << "chuncklen: " << recv << std::endl;
                        json.append(data+cpos,recv);
                        cpos+=(recv-1);
                        chunklen-=recv;
                    }

                    std::cerr << "recv: " << recv << std::endl;

                    if(chunklen!=0){
                        for(;;){
                            recv=srvsock->recvData(cltsock,data,16384);
                            if(recv>0)
                                break;
                            if(tries>5){
                                netplus::NetException e;
                                e[netplus::NetException::Error] << "nginxfiler: can't reach nginx server !";
                                throw e;
                            }
                            ++tries;
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        }
                        cpos=0;
                        goto READCHUNK;
                    }

                    if((chunklen=readchunk(data,recv,cpos)) > 0)
                        goto READCHUNK;
                }

                std::cout << json << std::endl;

                struct json_object *ndir;
                ndir = json_tokener_parse(json.c_str());

                if(!ndir){
                    libhttppp::HTTPException e;
                    e[libhttppp::HTTPException::Error] << "nginxfiler: counld't read json !";
                    throw e;
                }

                libhtmlpp::HtmlString fileHtml;

                RenderUI(path,ndir,fileHtml);

                std::string out,sid;
                page.getElementbyID("main")->insertChild(fileHtml.parse());

                Args->theme->printSite(out,page,req->getRequestURL(),Args->auth->isLoggedIn(req,sid));

                libhttppp::HttpResponse curres;
                curres.setVersion(HTTPVERSION(1.1));
                curres.setContentType("text/html");
                curres.setState(HTTP200);
                curres.send(curcon, out.c_str(),out.length());
            }catch(libhttppp::HTTPException &e){
                libhttppp::HttpResponse curres;
                curres.setVersion(HTTPVERSION(1.1));
                curres.setContentType("text/html");
                curres.setState(HTTP500);
                curres.send(curcon, e.what(),strlen(e.what()));
            }
            delete cltsock;
            delete srvsock;
            return true;
        }
    private:
        size_t readchunk(const char *data,size_t datasize,size_t &pos){
            size_t start=pos;
            while( (pos < datasize || pos < 512)&& data[pos++]!='\r');
            char value[512];

            if(pos-start > 512){
                return 0;
            }

            memcpy(value,data+start,pos-start);

            ++pos;

            return Hex2Int(value,nullptr);
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

