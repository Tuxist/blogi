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
        }

        ~Navbar(){
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

        void Rendering(libhttppp::HttpRequest *req,libhtmlpp::HtmlElement& curpage){
            blogi::SQL sql,sql2;
            blogi::DBResult res,res2;
            sql << "select id,name,container_id from navbar";

            std::string turl=req->getRequestURL();
            if(turl.rfind('?')>0){
                turl=turl.substr(0,turl.rfind('?'));
            }

            int n = Args->database->exec(&sql,res);
            if(n<1){
                libhttppp::HTTPException excep;
                excep[libhttppp::HTTPException::Critical] << "No entries found for navbar";
                throw excep;
            }
            libhtmlpp::HtmlString buf;
            for (int i = 0; i < n; i++) {
                buf << "<div id=\"" << res[0][1] << "\">" << "<ul>";

                sql2 << "select url,name from navbar_items WHERE navbar_id='" << res[i][0] << "' ORDER BY id";

                int n2 = Args->database->exec(&sql2,res2);
                for (int ii = 0; ii < n2; ii++) {
                    buf << "<li ";
                    if(turl.compare(0,strlen(res2[ii][0]),res2[ii][0]) == 0 )
                        buf << "class=\"active\"";
                    else
                        buf << "class=\"inactive\"";
                    buf << "><a href=\"" << res2[ii][0] << "\">" << res2[ii][1] << "</a></li>";
                }
                buf << "</ul></div>";
            }
            curpage.getElementbyID(res[0][2])->appendChild(buf.parse());
        }

        bool haveSettings(){
            return true;
        }

        void Settings(libhttppp::HttpRequest *req,libhtmlpp::HtmlString &setdiv){
            setdiv << "<div id=\"navsettings\"><span>Navbar Settings</span></div>";
        }

        bool Controller(netplus::con *curcon,libhttppp::HttpRequest *req,libhtmlpp::HtmlElement page){
            return false;
        }
    };
};

extern "C" blogi::PluginApi* create() {
    return new blogi::Navbar();
}

extern "C" void destroy(blogi::PluginApi* p) {
    delete p;
}

