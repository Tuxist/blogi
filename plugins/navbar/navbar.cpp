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

#include "plugin.h"
#include "database.h"
#include "theme.h"
#include "conf.h"

namespace blogi {
    class Navbar : public PluginApi {
    public:
        Navbar(){
            Navigation = new libhtmlpp::HtmlElement();
        }

        ~Navbar(){
            delete Navigation;
        }

        const char* getName(){
            return "navbar";
        }

        const char* getVersion(){
            return "0.1";
        }

        const char* getAuthor(){
            return "Jan Koester";
        }

        void initPlugin(){
            return;
        }

        void Rendering(libhtmlpp::HtmlElement* curpage){
            blogi::SQL sql;
            blogi::DBResult res;

            sql << "select url,name from navbar ORDER BY id";

            int n = Args->database->exec(&sql,res);
            if(n<1){
                libhttppp::HTTPException excep;
                excep[libhttppp::HTTPException::Critical] << "No entries found for navbar";
                throw excep;
            }
            libhtmlpp::HtmlString buf;
            buf << "<div id=\"navbar\">" << "<ul>";
            for (int i = 0; i < n; i++) {
                buf << "<li ";
                buf << "class=\"inactive\"";
                buf << "><a href=\"" << res[i][0] << "\">" << res[i][1] << "</a></li>";
            }
            //<li class=\"active\" style=\"padding:1px 10px; float:left\"><a href=\"" << _Config.config->buildurl(index,url,512) <<"\">Blog</a></li>
            buf << "</ul></div>";
            Navigation->appendChild(buf.parse());
            Args->theme->Rendering(curpage,"header",Navigation);
        }

        bool haveSettings(){
            return true;
        }

        void Settings(libhttppp::HttpRequest *req,libhtmlpp::HtmlString &setdiv){
            setdiv << "<div id=\"navsettings\"><span>Navbar Settings</span></div>";
        }

        bool Controller(netplus::con *curcon,libhttppp::HttpRequest *req){
            std::string turl=req->getRequestURL();
            if(turl.rfind('?')>0){
                turl=turl.substr(0,turl.rfind('?'));
            }

            // if(turl.compare(0,strlen(res[i][0]),res[i][0]) == 0 )
            // //        buf << "class=\"active\"";
            //     else
            //
            return false;
        }
    private:
        libhtmlpp::HtmlElement *Navigation;
    };
};

extern "C" blogi::PluginApi* create() {
    return new blogi::Navbar();
}

extern "C" void destroy(blogi::PluginApi* p) {
    delete p;
}

