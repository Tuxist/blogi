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

const unsigned char GOOGLECA[]="\
-----BEGIN CERTIFICATE-----\n\
MIIFVzCCAz+gAwIBAgINAgPlk28xsBNJiGuiFzANBgkqhkiG9w0BAQwFADBHMQsw\n\
CQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZpY2VzIExMQzEU\n\
MBIGA1UEAxMLR1RTIFJvb3QgUjEwHhcNMTYwNjIyMDAwMDAwWhcNMzYwNjIyMDAw\n\
MDAwWjBHMQswCQYDVQQGEwJVUzEiMCAGA1UEChMZR29vZ2xlIFRydXN0IFNlcnZp\n\
Y2VzIExMQzEUMBIGA1UEAxMLR1RTIFJvb3QgUjEwggIiMA0GCSqGSIb3DQEBAQUA\n\
A4ICDwAwggIKAoICAQC2EQKLHuOhd5s73L+UPreVp0A8of2C+X0yBoJx9vaMf/vo\n\
27xqLpeXo4xL+Sv2sfnOhB2x+cWX3u+58qPpvBKJXqeqUqv4IyfLpLGcY9vXmX7w\n\
Cl7raKb0xlpHDU0QM+NOsROjyBhsS+z8CZDfnWQpJSMHobTSPS5g4M/SCYe7zUjw\n\
TcLCeoiKu7rPWRnWr4+wB7CeMfGCwcDfLqZtbBkOtdh+JhpFAz2weaSUKK0Pfybl\n\
qAj+lug8aJRT7oM6iCsVlgmy4HqMLnXWnOunVmSPlk9orj2XwoSPwLxAwAtcvfaH\n\
szVsrBhQf4TgTM2S0yDpM7xSma8ytSmzJSq0SPly4cpk9+aCEI3oncKKiPo4Zor8\n\
Y/kB+Xj9e1x3+naH+uzfsQ55lVe0vSbv1gHR6xYKu44LtcXFilWr06zqkUspzBmk\n\
MiVOKvFlRNACzqrOSbTqn3yDsEB750Orp2yjj32JgfpMpf/VjsPOS+C12LOORc92\n\
wO1AK/1TD7Cn1TsNsYqiA94xrcx36m97PtbfkSIS5r762DL8EGMUUXLeXdYWk70p\n\
aDPvOmbsB4om3xPXV2V4J95eSRQAogB/mqghtqmxlbCluQ0WEdrHbEg8QOB+DVrN\n\
VjzRlwW5y0vtOUucxD/SVRNuJLDWcfr0wbrM7Rv1/oFB2ACYPTrIrnqYNxgFlQID\n\
AQABo0IwQDAOBgNVHQ8BAf8EBAMCAYYwDwYDVR0TAQH/BAUwAwEB/zAdBgNVHQ4E\n\
FgQU5K8rJnEaK0gnhS9SZizv8IkTcT4wDQYJKoZIhvcNAQEMBQADggIBAJ+qQibb\n\
C5u+/x6Wki4+omVKapi6Ist9wTrYggoGxval3sBOh2Z5ofmmWJyq+bXmYOfg6LEe\n\
QkEzCzc9zolwFcq1JKjPa7XSQCGYzyI0zzvFIoTgxQ6KfF2I5DUkzps+GlQebtuy\n\
h6f88/qBVRRiClmpIgUxPoLW7ttXNLwzldMXG+gnoot7TiYaelpkttGsN/H9oPM4\n\
7HLwEXWdyzRSjeZ2axfG34arJ45JK3VmgRAhpuo+9K4l/3wV3s6MJT/KYnAK9y8J\n\
ZgfIPxz88NtFMN9iiMG1D53Dn0reWVlHxYciNuaCp+0KueIHoI17eko8cdLiA6Ef\n\
MgfdG+RCzgwARWGAtQsgWSl4vflVy2PFPEz0tv/bal8xa5meLMFrUKTX5hgUvYU/\n\
Z6tGn6D/Qqc6f1zLXbBwHSs09dR2CQzreExZBfMzQsNhFRAbd03OIozUhfJFfbdT\n\
6u9AWpQKXCBfTkBdYiJ23//OYb2MI3jSNwLgjt7RETeJ9r/tSQdirpLsQBqvFAnZ\n\
0E6yove+7u7Y/9waLd64NnHi/Hm3lCXRSHNboTXns5lndcEZOitHTtNCjv0xyBZm\n\
2tIMPNuzjsmhDYAPexZ3FL//2wmUspO8IFgV6dtxQ/PeEMMA3KgqlbbC1j+Qa3bb\n\
bP6MvPJwNQzcmRk13NfIRmPVNnGuV/u3gm3c\n\
-----END CERTIFICATE-----\n\
\0";

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
                << ");"
                << " CREATE TABLE IF NOT EXISTS youtube_videos ("
                <<   "id integer PRIMARY KEY " << Args->database->autoincrement() << ","
                <<   "youtube_channels_pkey bigint REFERENCES youtube_channels(id),"
                <<   "youtube_id varchar(255) NOT NULL"
                << ")";

            Args->database->exec(&sql,res);

        }

        void SyncYoutube(int channel_id,libhtmlpp::HtmlString &out){
            std::string json;
            libhttppp::HttpResponse res;
            size_t hsize=0,cpos;
            bool chunked=false;
            blogi::SQL        sql;
            blogi::DBResult   dbres;

            int rlen=0;

            sql << "select channelname,apikey,channelkey from youtube_channels where id='" << channel_id <<"'";

            int count = Args->database->exec(&sql,dbres);

            if(count < 1){
                libhttppp::HTTPException he;
                he[libhttppp::HTTPException::Error] << "Youtube channel id not found in database !";
                throw he;
            }

            try{
                std::shared_ptr<netplus::ssl> ysock=std::make_shared<netplus::ssl>("www.googleapis.com",443,1,0,GOOGLECA,1966);
                std::shared_ptr<netplus::ssl> ycsock=std::make_shared<netplus::ssl>();

                ysock->connect(ycsock);
                ycsock->setnonblocking();

                libhttppp::HttpRequest nreq;
                nreq.setRequestType(GETREQUEST);
                std::string churl="/youtube/v3/search?key="; churl+= dbres[0][1];
                churl+="&channelId="; churl+=dbres[0][2]; churl+="&part=snippet,id&order=date";
                nreq.setRequestURL(churl.c_str());
                nreq.setRequestVersion(HTTPVERSION(1.1));
                *nreq.setData("connection") << "keep-alive";
                *nreq.setData("host")  << "www.googleapis.com" << ":" << 443;
                *nreq.setData("accept") << "text/json";
                *nreq.setData("user-agent") << "blogi/1.0 (Alpha Version 0.1)";
                nreq.send(ycsock,ysock);

                char data[16384];
                int recv,tries=0,chunklen=0;

                try{
                    for(;;){
                        try{
                            recv=ysock->recvData(ycsock,data,16384);
                            break;
                        }catch(netplus::NetException &re){
                            if(re.getErrorType()!=netplus::NetException::Note)
                                throw re;
                            else
                                continue;
                        }
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
                                    for(;;){
                                        try{
                                            recv=ysock->recvData(ycsock,data,16384);
                                            break;
                                        }catch(netplus::NetException &re){
                                            if(re.getErrorType()!=netplus::NetException::Note)
                                                throw re;
                                            else
                                                continue;
                                        }
                                    }
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
                            cpos=0;
                            for(;;){
                                try{
                                    recv=ysock->recvData(ycsock,data,16384);
                                    break;
                                }catch(netplus::NetException &e){
                                    if(e.getErrorType()!=netplus::NetException::Note){
                                        libhttppp::HTTPException ee;
                                        ee[libhttppp::HTTPException::Error] << e.what();
                                        throw ee;
                                    }else{
                                        continue;
                                    }
                                }
                            }
                        }
                    };
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

            try{
                enum json_type type = json_object_get_type(youindex);
                struct json_object *ytems;
                ytems = json_object_object_get(youindex,"items");
                int fcount = json_object_array_length(ytems);

                sql << "delete from youtube_videos where channel_id='" << channel_id <<"';";
                Args->database->exec(&sql,dbres);

                for(int i =0; i<fcount; ++i) {
                    int ii=0;
                    json_object_object_foreach(json_object_array_get_idx(ytems,i), key, val) {
                        if(strcmp(key,"id")==0 && val){
                            struct json_object *yid;
                            yid = json_object_object_get(val,"videoId");
                            if(yid){
                                sql.clear();
                                sql << "INSERT INTO youtube_videos (youtube_channels_pkey,youtube_id) VALUES ('" << channel_id << "','" << json_object_get_string(yid) << "')";
                                Args->database->exec(&sql,dbres);
                            }
                        }
                    }
                }
                out << "<span>" << fcount << " videos added !" << "</span>";
                return;
            }catch(libhtmlpp::HTMLException &e){
                libhttppp::HTTPException ee;
                ee[libhttppp::HTTPException::Error] << e.what();
                throw ee;
            }
        }

        bool Controller(libhttppp::HttpRequest * req,libhtmlpp::HtmlElement *page){
            char url[512];
            libhtmlpp::HtmlString out;
            libhtmlpp::HtmlElement *hmain, youdiv("div");

            youdiv.setAttribute("id","youtube-pl");

            std::string sid;
            if(strncmp(req->getRequestURL(),Args->config->buildurl("youtube",url,512),strlen(Args->config->buildurl("youtube",url,512)))!=0){
                return false;
            }

            if(strcmp(req->getRequestURL(),Args->config->buildurl("youtube/sync/",url,512))>0){
                if(Args->auth->isLoggedIn(req,sid)){
                    try{
                        SyncYoutube( atoi( req->getRequestURL()+strlen(Args->config->buildurl("youtube/sync/",url,512)) ),out);
                        youdiv.appendChild(out.parse());
                    }catch(libhttppp::HTTPException &e){
                        std::string error;
                        libhtmlpp::HtmlString msg;
                        libhtmlpp::HtmlEncode(e.what(),error);
                        msg << "<span>" << error << "</span>";
                        youdiv.appendChild(msg.parse());
                    }
                }else{
                        libhtmlpp::HtmlString msg;
                        msg << "<span> Insufficient rights can't sync youtube videos ! </span>";
                        youdiv.appendChild(msg.parse());
                }
                if( (hmain=page->getElementbyID("main")) )
                        hmain->appendChild(&youdiv);
                Args->theme->printSite(out,page,req->getRequestURL(),Args->auth->isLoggedIn(req,sid));
                libhttppp::HttpResponse curres;
                curres.setVersion(HTTPVERSION(1.1));
                curres.setContentType("text/html");
                curres.setState(HTTP200);
                curres.send(req, out.c_str(),out.size());
                return true;

            }
            blogi::SQL        sql;
            blogi::DBResult   dbres;

            sql << "SELECT youtube_channels.channelname,youtube_videos.youtube_id "
                << "FROM youtube_channels LEFT JOIN youtube_videos ON youtube_channels.id = youtube_videos.youtube_channels_pkey;";

            int count = Args->database->exec(&sql,dbres);

            if(count < 1){
                libhttppp::HTTPException ee;
                ee[libhttppp::HTTPException::Error] << "no youtube channels or videos found please sync!";
                throw ee;
            }

            libhtmlpp::HtmlElement youul("ul");

            for(int i=0; i<count; ++i){
                libhtmlpp::HtmlString youmain;
                youmain << "<li><iframe class=\"ytplayer\" width=\"640\" height=\"360\" src=\"http://www.youtube.com/embed/"
                        << dbres[i][1]  << "?autoplay=1&hd=1\" frameborder=\"0\" allowFullscreen> </iframe></li>";
                youul.appendChild(youmain.parse());
            }

            youdiv.appendChild(&youul);

            if( (hmain=page->getElementbyID("main")) )
                hmain->appendChild(&youdiv);

            Args->theme->printSite(out,page,req->getRequestURL(),Args->auth->isLoggedIn(req,sid));

            libhttppp::HttpResponse curres;
            curres.setVersion(HTTPVERSION(1.1));
            curres.setContentType("text/html");
            curres.setState(HTTP200);
            curres.send(req, out.c_str(),out.size());

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
    };
};

extern "C" blogi::PluginApi* create() {
    return new blogi::Youtube();
}

extern "C" void destroy(blogi::PluginApi* p) {
    delete p;
}
