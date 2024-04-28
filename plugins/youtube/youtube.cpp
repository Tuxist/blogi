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

#include <cstring>
#include <string>
#include <vector>
#include <chrono>
#include <thread>

#include <netplus/socket.h>
#include <netplus/exception.h>

#include <htmlpp/html.h>
#include <htmlpp/exception.h>

#include <json-c/json.h>
#include <json-c/json_object.h>

#include "plugin.h"
#include "database.h"
#include "theme.h"
#include "conf.h"

#define CHANNELID UC9WqtMjJL6VHuDqq9endmEA

#include <iostream>

const unsigned char GOOGLECA[]=\
"-----BEGIN CERTIFICATE-----\n\
MIIFljCCA36gAwIBAgINAgO8U1lrNMcY9QFQZjANBgkqhkiG9w0BAQsFADBHMQsw\n\
CQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEU\n\
MBIGA1UEAxMLR1RTIFJvb3QgUjEwHhcNMjAwODEzMDAwMDQyWhcNMjcwOTMwMDAw\n\
MDQyWjBGMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZp\n\
Y2VzIExMQzETMBEGA1UEAxMKR1RTIENBIDFDMzCCASIwDQYJKoZIhvcNAQEBBQAD\n\
ggEPADCCAQoCggEBAPWI3+dijB43+DdCkH9sh9D7ZYIl/ejLa6T/belaI+KZ9hzp\n\
kgOZE3wJCor6QtZeViSqejOEH9Hpabu5dOxXTGZok3c3VVP+ORBNtzS7XyV3NzsX\n\
lOo85Z3VvMO0Q+sup0fvsEQRY9i0QYXdQTBIkxu/t/bgRQIh4JZCF8/ZK2VWNAcm\n\
BA2o/X3KLu/qSHw3TT8An4Pf73WELnlXXPxXbhqW//yMmqaZviXZf5YsBvcRKgKA\n\
gOtjGDxQSYflispfGStZloEAoPtR28p3CwvJlk/vcEnHXG0g/Zm0tOLKLnf9LdwL\n\
tmsTDIwZKxeWmLnwi/agJ7u2441Rj72ux5uxiZ0CAwEAAaOCAYAwggF8MA4GA1Ud\n\
DwEB/wQEAwIBhjAdBgNVHSUEFjAUBggrBgEFBQcDAQYIKwYBBQUHAwIwEgYDVR0T\n\
AQH/BAgwBgEB/wIBADAdBgNVHQ4EFgQUinR/r4XN7pXNPZzQ4kYU83E1HScwHwYD\n\
VR0jBBgwFoAU5K8rJnEaK0gnhS9SZizv8IkTcT4waAYIKwYBBQUHAQEEXDBaMCYG\n\
CCsGAQUFBzABhhpodHRwOi8vb2NzcC5wa2kuZ29vZy9ndHNyMTAwBggrBgEFBQcw\n\
AoYkaHR0cDovL3BraS5nb29nL3JlcG8vY2VydHMvZ3RzcjEuZGVyMDQGA1UdHwQt\n\
MCswKaAnoCWGI2h0dHA6Ly9jcmwucGtpLmdvb2cvZ3RzcjEvZ3RzcjEuY3JsMFcG\n\
A1UdIARQME4wOAYKKwYBBAHWeQIFAzAqMCgGCCsGAQUFBwIBFhxodHRwczovL3Br\n\
aS5nb29nL3JlcG9zaXRvcnkvMAgGBmeBDAECATAIBgZngQwBAgIwDQYJKoZIhvcN\n\
AQELBQADggIBAIl9rCBcDDy+mqhXlRu0rvqrpXJxtDaV/d9AEQNMwkYUuxQkq/BQ\n\
cSLbrcRuf8/xam/IgxvYzolfh2yHuKkMo5uhYpSTld9brmYZCwKWnvy15xBpPnrL\n\
RklfRuFBsdeYTWU0AIAaP0+fbH9JAIFTQaSSIYKCGvGjRFsqUBITTcFTNvNCCK9U\n\
+o53UxtkOCcXCb1YyRt8OS1b887U7ZfbFAO/CVMkH8IMBHmYJvJh8VNS/UKMG2Yr\n\
PxWhu//2m+OBmgEGcYk1KCTd4b3rGS3hSMs9WYNRtHTGnXzGsYZbr8w0xNPM1IER\n\
lQCh9BIiAfq0g3GvjLeMcySsN1PCAJA/Ef5c7TaUEDu9Ka7ixzpiO2xj2YC/WXGs\n\
Yye5TBeg2vZzFb8q3o/zpWwygTMD0IZRcZk0upONXbVRWPeyk+gB9lm+cZv9TSjO\n\
z23HFtz30dZGm6fKa+l3D/2gthsjgx0QGtkJAITgRNOidSOzNIb2ILCkXhAd4FJG\n\
AJ2xDx8hcFH1mt0G/FX0Kw4zd8NLQsLxdxP8c4CU6x+7Nz/OAipmsHMdMqUybDKw\n\
juDEI/9bfU1lcKwrmz3O2+BtjjKAvpafkmO8l7tdufThcV4q5O8DIrGKZTqPwJNl\n\
1IXNDw9bg1kWRxYtnCQ6yICmJhSFm/Y3m6xv+cXDBlHz4n/FsRC6UfTd\n\
-----END CERTIFICATE-----\0";

namespace blogi {
    class Youtube : public PluginApi {
    public:
        Youtube(){
        }
        const char* getName(){
            return "youtube";
        }

        const char* getVersion(){
            return "0.1";
        }

        const char* getAuthor(){
            return "Jan Koester";
        }

        void initPlugin(){
            blogi::SQL        sql;
            blogi::DBResult   res;


            sql << "CREATE TABLE IF NOT EXISTS youtube_channels("
                <<   "id integer PRIMARY KEY " << Args->database->autoincrement() << ","
                <<   "channelname character varying(255) NOT NULL,"
                <<   "apikey character varying(255) NOT NULL,"
                <<   "channelkey character varying(255) NOT NULL"
                << ");";

            Args->database->exec(&sql,res);

            sql="select channelname,apikey,channelkey from youtube_channels";

            int count = Args->database->exec(&sql,res);

            for (int i = 0; i < count; i++) {
                Channel chan;
                chan.Name=res[i][0];
                chan.ApiKey=res[i][1];
                chan.Key=res[i][2];
                channels.push_back(chan);
            }
        }

        bool Controller(netplus::con * curcon, libhttppp::HttpRequest * req,libhtmlpp::HtmlElement *page){
            char url[512];
            if(strncmp(req->getRequestURL(),Args->config->buildurl("youtube",url,512),strlen(Args->config->buildurl("youtube",url,512)))!=0){
                return false;
            }

            std::string json;
            libhttppp::HttpResponse res;
            size_t hsize=0,cpos;
            bool chunked=false;

            int rlen=0;

            try{
                netplus::ssl ysock("www.googleapis.com",443,1,0,GOOGLECA,1966),ycsock;
                ysock.connect(&ycsock);
                ycsock.setnonblocking();

                for(auto channel : channels){
                    libhttppp::HttpRequest nreq;
                    nreq.setRequestType(GETREQUEST);
                    std::string churl="/youtube/v3/search?key="; churl+= channel.ApiKey.c_str();
                    churl+="&channelId="; churl+=channel.Key; churl+="&part=snippet,id&order=date&maxResults=20";
                    nreq.setRequestURL(churl.c_str());
                    nreq.setRequestVersion(HTTPVERSION(1.1));
                    *nreq.setData("connection") << "keep-alive";
                    *nreq.setData("host")  << "www.googleapis.com" << ":" << 443;
                    *nreq.setData("accept") << "text/json";
                    *nreq.setData("user-agent") << "blogi/1.0 (Alpha Version 0.1)";
                    nreq.send(&ycsock,&ysock);

                    char data[16384];
                    int recv,tries=0,chunklen=0;
                    try{
                        for(;;){
                            recv=ysock.recvData(&ycsock,data,16384);
                            if(recv>0)
                                break;
                            // if(tries>5){
                            //     netplus::NetException e;
                            //     e[netplus::NetException::Error] << "youtube-pl: can't reach youtube server !";
                            //     throw e;
                            // }
                            std::this_thread::sleep_for(std::chrono::milliseconds(100*tries));
                            ++tries;
                        }
                    }catch(netplus::NetException &e){
                        libhttppp::HTTPException he;
                        he[libhttppp::HTTPException::Error] << e.what();
                        throw he;
                    }

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
                                        recv=ysock.recvData(&ycsock,data,16384);
                                        if(recv>0)
                                            break;
                                        // if(tries>5){
                                        //     netplus::NetException e;
                                        //     e[netplus::NetException::Error] << "nginxfiler: can't reach nginx server !";
                                        //     throw e;
                                        // }
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
                                        recv=ysock.recvData(&ycsock,data,16384);
                                        cpos=0;
                                        if(recv == 0 && tries>10){
                                            netplus::NetException e;
                                            e[netplus::NetException::Error] << "nginxfiler: can't reach nginx server !";
                                            throw e;
                                        }else if(recv !=0 ){
                                            break;
                                        }
                                        ++tries;
                                        std::this_thread::sleep_for(std::chrono::milliseconds(100*tries));

                                    }
                                }catch(netplus::NetException &e){
                                    libhttppp::HTTPException ee;
                                    ee[libhttppp::HTTPException::Error] << e.what();
                                    throw ee;
                                }
                            }
                        };
                    }
                }
            }catch(netplus::NetException &e){
                libhttppp::HTTPException ee;
                ee[libhttppp::HTTPException::Error] << e.what();
                throw ee;
            }

            libhtmlpp::HtmlElement youmain("ul");


            std::cout << json << std::endl;

            struct json_object *youindex;
            youindex = json_tokener_parse(json.c_str());

            if(!youindex){
                libhttppp::HTTPException e;
                e[libhttppp::HTTPException::Error] << "nginxfiler: counld't read json !";
                throw e;
            }

            libhtmlpp::HtmlString cHtml;

            try{
                enum json_type type = json_object_get_type(youindex);

                if(type==json_type_array){

                    struct json_object *ytems;
                    ytems = json_object_object_get(youindex,"items");
                    int fcount = json_object_array_length(ytems);

                    for(int i =0; i<fcount; ++i) {
                        int ntype=-1;
                        json_object_object_foreach(json_object_array_get_idx(ytems,i), key, val) {
                            if(strcmp(key,"id")==0 && val){
                                struct json_object *yid;
                                yid = json_object_object_get(ytems,"videoID");
                                if(yid){
                                    cHtml << "<li><iframe id=\"ytplayer\" type=\"text/html\" width=\"640\" height=\"360\" src=\"http://www.youtube.com/embed/"
                                    << json_object_get_string(yid)  << "?autoplay=1\" frameborder=\"0\" allowFullscreen> </iframe></li>";
                                }
                            }
                        }
                    }

                    if(!cHtml.empty()){
                        libhtmlpp::HtmlElement *fel=cHtml.parse();

                        if(fel)
                            youmain.appendChild(fel);
                    }

                }
            }catch(libhtmlpp::HTMLException &e){
                libhttppp::HTTPException ee;
                ee[libhttppp::HTTPException::Error] << e.what();
                throw ee;
            }


            libhtmlpp::HtmlString out;
            std::string sid;

            libhtmlpp::HtmlElement *hmain, youdiv("div");
            youdiv.setAttribute("id","youtube-pl");

            youdiv.appendChild(&youmain);

            if( (hmain=page->getElementbyID("main")) )
                hmain->appendChild(&youdiv);

            Args->theme->printSite(out,page,req->getRequestURL(),Args->auth->isLoggedIn(req,sid));

            libhttppp::HttpResponse curres;
            curres.setVersion(HTTPVERSION(1.1));
            curres.setContentType("text/html");
            curres.setState(HTTP200);
            curres.send(curcon, out.c_str(),out.size());

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

        struct Channel {
            std::string Name;
            std::string ApiKey;
            std::string Key;
        };

        std::vector<Channel> channels;
    };
};

extern "C" blogi::PluginApi* create() {
    return new blogi::Youtube();
}

extern "C" void destroy(blogi::PluginApi* p) {
    delete p;
}
