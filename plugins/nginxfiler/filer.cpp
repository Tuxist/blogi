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

#include <fstream>
#include <cstring>
#include <string>
#include <vector>
#include <unistd.h>
#include <sys/utsname.h>

#include "plugin.h"
#include "database.h"
#include "theme.h"
#include "conf.h"

#include <httppp/http.h>
#include <httppp/exception.h>

#include <netplus/exception.h>

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
            Config::ConfigData *url=Args->config->getKey("/BLOGI/NGINXFILER/URL");
            if(Args->config->getValue(url,0)){
                _Url=Args->config->getValue(url,0);
            }else{
                libhttppp::HTTPException err;
                err[libhttppp::HTTPException::Error] << "No Nginx url found in Config Aborting!";
                throw err;
            }
        }

        bool Controller(netplus::con *curcon,libhttppp::HttpRequest *req,libhtmlpp::HtmlElement page){
            char url[512];
            if(strncmp(req->getRequestURL(),Args->config->buildurl("nginxfiler",url,512),strlen(Args->config->buildurl("nginxfiler",url,512)))!=0){
                return false;
            }
            netplus::tcp *srvsock;
            netplus::socket *cltsock=nullptr;
            try{
                char host[255];
                sscanf(_Url.c_str(),"http://%s/*",host);
                try{
                    srvsock= new netplus::tcp("10.1.2.207",80,2,0);
                    cltsock=srvsock->connect();
                }catch(netplus::NetException &e){
                    libhttppp::HTTPException he;
                    he[libhttppp::HTTPException::Error] << e.what();
                    throw he;
                }

                libhttppp::HttpRequest nreq;
                nreq.setRequestType(GETREQUEST);
                size_t udel=_Url.rfind("/");
                if(udel==std::string::npos){
                    libhttppp::HTTPException exp;
                    exp[libhttppp::HTTPException::Error] << "nginx filer Url incoreect !";
                    throw exp;
                }
                nreq.setRequestURL("/files");
                nreq.setRequestVersion(HTTPVERSION(1.1));
                *nreq.setData("connection") << "keep-alive";
                *nreq.setData("host")  << "tuxist.de";
                *nreq.setData("accept") << "text/json";
                *nreq.setData("user-agent") << "blogi/1.0 (Alpha Version 0.1)";
                nreq.send(cltsock,srvsock);

                char data[16384];
                size_t recv;

                try{
                    recv=srvsock->recvData(cltsock,data,16384);
                }catch(netplus::NetException &e){
                    libhttppp::HTTPException he;
                    he[libhttppp::HTTPException::Error] << e.what();
                    throw he;
                }



                std::string json;
                libhttppp::HttpResponse res;
                size_t len=recv,chunklen=0,hsize=0;

                int rlen=0;


                hsize=res.parse(data,len);
                if(strcmp(res.getTransferEncoding(),"chunked")==0){
                    chunklen=readchunk(data,recv,--hsize);
                }else{
                    rlen=res.getContentLength();
                    json.resize(rlen);
                }

                try{
                    if(chunklen==0){
                        do{
                            json.append(data+hsize,recv-hsize);
                            rlen-=recv-hsize;
                            if(rlen>0){
                                recv=srvsock->recvData(cltsock,data,16384);
                                hsize=0;
                            }
                        }while(rlen>0);
                    }else{
                        size_t cpos=hsize;
                        do{
                            json.append(data+cpos,chunklen);
                            cpos+=chunklen;
                            if(recv<chunklen){
                                recv=srvsock->recvData(cltsock,data,16384);
                                cpos=0;
                            }
                        }while((chunklen=readchunk(data,recv,cpos))!=0);
                    }
                }catch(netplus::NetException &e){
                    libhttppp::HTTPException he;
                    he[libhttppp::HTTPException::Error] << e.what();
                    throw he;
                }

                delete cltsock;
                delete srvsock;

                libhttppp::HttpResponse curres;
                curres.setVersion(HTTPVERSION(1.1));
                curres.setContentType("text/json");
                curres.setState(HTTP200);
                curres.send(curcon, json.c_str(),json.length());
            }catch(libhttppp::HTTPException &e){
                delete cltsock;
                delete srvsock;
                libhttppp::HttpResponse curres;
                curres.setVersion(HTTPVERSION(1.1));
                curres.setContentType("text/json");
                curres.setState(HTTP200);
                curres.send(curcon, e.what());
            }
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

        std::string _Url;
    };
};

extern "C" blogi::PluginApi* create() {
    return new blogi::NginxFiler();
}

extern "C" void destroy(blogi::PluginApi* p) {
    delete p;
}

