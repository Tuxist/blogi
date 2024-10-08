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

#ifdef Linux
#include "icons/tux.h"
#else
#include "icons/freebsd.h"
#endif

#define RELEASEFILE "/etc/os-release"

namespace blogi {
    class NodeInfo : public PluginApi {
    public:
        NodeInfo(){
        }
        const char* getName(){
            return "nodeinfo";
        }

        const char* getVersion(){
            return "0.1";
        }

        const char* getAuthor(){
            return "Jan Koester";
        }

        void initPlugin(){
        }

        bool Controller(const int tid,libhttppp::HttpRequest *req,libhtmlpp::HtmlElement *page){
            char url[512];
            if(strncmp(req->getRequestURL(),Args->config->buildurl("nodeinfo",url,512),strlen(Args->config->buildurl("nodeinfo",url,512)))!=0){
                return false;
            }

            if(strncmp(req->getRequestURL(),Args->config->buildurl("nodeinfo/sysico.png",url,512),strlen(Args->config->buildurl("nodeinfo/sysico.png",url,512)))==0){
                libhttppp::HttpResponse resp;
                resp.setVersion(HTTPVERSION(1.1));
                resp.setState(HTTP200);
                resp.setContentType("image/jpeg");
                resp.send(req,(const char*)sysicon,sysicon_size);
                return true;
            }

            libhtmlpp::HtmlString condat,sysicohtml;

            sysicohtml << "<img alt=\"sysico\" src=\"" << Args->config->buildurl("nodeinfo/sysico.png",url,512) << "\" />";

            #ifndef Windows
            struct utsname usysinfo;
            uname(&usysinfo);
            /*create htmltable widget*/
            libhtmlpp::HtmlTable htmltable;


            char hostname[255];

            if(gethostname(hostname,255)==0){
                htmltable << libhtmlpp::HtmlTable::Row() << "Hostname: " << hostname;
            }

            /*create table rows*/
            htmltable << libhtmlpp::HtmlTable::Row() << "Operating system:" << (sysicohtml << usysinfo.sysname);
            htmltable << libhtmlpp::HtmlTable::Row() << "Release Version :" << usysinfo.release;
            htmltable << libhtmlpp::HtmlTable::Row() << "Hardware        :" << usysinfo.machine;

            std::fstream osr(RELEASEFILE,std::ios::in);
            std::string osline;

            if(osr.is_open()){
                while ( getline (osr,osline) ){
                    std::string name,entry;
                    int deli=osline.find('=');
                    name=osline.substr(0,deli);
                    size_t sts=osline.find('\"',deli);
                    size_t ets=osline.find('\"',++sts);
                    if(sts!=std::string::npos && ets!=std::string::npos)
                        entry=osline.substr(sts,ets-sts);
                    htmltable << libhtmlpp::HtmlTable::Row() << name.c_str() << entry.c_str();
                }
            }

#ifdef Linux
            long psize;
            if((psize=sysconf(_SC_PAGESIZE))>0){
                char ab[512];
                snprintf(ab,512,"%ld/%ld MB",(sysconf(_SC_AVPHYS_PAGES)*psize)/(1048576),((sysconf(_SC_PHYS_PAGES)*psize)/1048576));
                htmltable << libhtmlpp::HtmlTable::Row() << "Arbeitspeicher: " <<  ab;
            }
#endif

            libhtmlpp::HtmlElement table;

            htmltable.insert(&table);
            table.setAttribute("class","kinfo");

            libhtmlpp::HtmlElement html;
            html.setTagname("div");;
            html.setAttribute("id","content");
            html.insertChild(&table);

            libhtmlpp::HtmlString out,systable;
            std::string sid;
            libhtmlpp::print(&html,systable);

            condat << systable;

            page->getElementbyID("main")->insertChild(condat.parse());

            Args->theme->printSite(tid,out,page,req->getRequestURL(),
                                    Args->auth->isLoggedIn(tid,req,sid));

            #endif

            libhttppp::HttpResponse resp;
            resp.setVersion(HTTPVERSION(1.1));
            resp.setState(HTTP200);
            resp.setContentType("text/html");
            resp.send(req,out.c_str(),out.size());
            return true;

        }
    };
};

extern "C" blogi::PluginApi* create() {
    return new blogi::NodeInfo();
}

extern "C" void destroy(blogi::PluginApi* p) {
    delete p;
}
