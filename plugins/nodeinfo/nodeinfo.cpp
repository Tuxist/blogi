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
#include <unistd.h>
#include <sys/utsname.h>

#include "plugin.h"
#include "database.h"
#include "theme.h"
#include "conf.h"

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

        bool Controller(netplus::con *curcon,libhttppp::HttpRequest *req){
            char url[512];
            if(strncmp(req->getRequestURL(),Args->config->buildurl("nodeinfo",url,512),strlen(Args->config->buildurl("nodeinfo",url,512)))!=0){
                return false;
            }
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
            htmltable << libhtmlpp::HtmlTable::Row() << "Operating system:" << usysinfo.sysname;
            htmltable << libhtmlpp::HtmlTable::Row() << "Release Version :" << usysinfo.release;
            htmltable << libhtmlpp::HtmlTable::Row() << "Hardware        :" << usysinfo.machine;

            long psize;
            if((psize=sysconf(_SC_PAGESIZE))>0){
                char ab[512];
                snprintf(ab,512,"%ld/%ld MB",(sysconf(_SC_AVPHYS_PAGES)*psize)/(1048576),((sysconf(_SC_PHYS_PAGES)*psize)/1048576));
                htmltable << libhtmlpp::HtmlTable::Row() << "Arbeitspeicher: " <<  ab;
            }
            libhtmlpp::HtmlElement table;
            htmltable.insert(&table);
            table.setAttribute("class","kinfo");

            libhtmlpp::HtmlElement html;
            html.setTagname("div");;
            html.setAttribute("id","content");
            html.insertChild(&table);

            std::string out,sid,systable;
            libhtmlpp::print(&html,nullptr,systable);

            libhtmlpp::HtmlString condat;
            condat << systable;

            Args->theme->RenderSite(out,req->getRequestURL(),condat,
                                    Args->auth->isLoggedIn(req,sid));

            #endif

            libhttppp::HttpResponse resp;
            resp.setVersion(HTTPVERSION(1.1));
            resp.setState(HTTP200);
            resp.setContentType("text/html");
            resp.send(curcon,out.c_str(),out.length());
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
