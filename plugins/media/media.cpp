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

#include <htmlpp/html.h>
#include <httppp/http.h>

#include <plugin.h>

#include "icon.webp.h"

namespace blogi {
    class Media : public PluginApi {
    public:
        Media(){
        }
        const char* getName(){
            return "media";
        }

        const char* getVersion(){
            return "0.1";
        }

        const char* getAuthor(){
            return "Jan Koester";
        }

        bool haveSettings(){
            return true;
        }

        void newPage(libhttppp::HttpRequest * req, libhtmlpp::HtmlString & setdiv){
            int id = -1;
            std::string albumname;
            blogi::SQL sql,sql2;
            blogi::DBResult res,res2;

            sql << "select id,username from users";

            libhttppp::HttpForm form;

            form.parse(req);

            for(libhttppp::HttpForm::UrlcodedFormData *curdat=form.getUrlcodedFormData(); curdat; curdat=curdat->nextUrlcodedFormData()){
                if(strcmp(curdat->getKey(),"userid")==0)
                    id=atoi(curdat->getValue());
                else if(strcmp(curdat->getKey(),"albumname")==0)
                    albumname=curdat->getValue();
            }

            if(id >0 && !albumname.empty()){
                time_t t = time(NULL);
                struct tm time = { 0 };
                char ttmp[26];
                localtime_r(&t,&time);
                asctime_r(&time,ttmp);
                sql2 << "INSERT INTO media_albums (name,owner,created) VALUES ('"; sql2.escaped(albumname.c_str()) << "','" << id << "','" << ttmp<< "')";
                Args->database->exec(&sql2,res2);
            }

            setdiv << "<form method=\"POST\" ><span>Albumname</span>:<input name=\"albumname\" type=\"text\" />"
                   << "<select name=\"userid\" >";
            int n = Args->database->exec(&sql,res);
            for(int i=0; i <n; ++i){
                setdiv << "<option value=\"" << res[i][0] <<"\">" << res[i][1] << "</option>";
            }
            setdiv << "</select>"
                   << "<input value=\"create\" type=\"submit\" /></form>";
        }

        void editAlbum(libhttppp::HttpRequest * req, libhtmlpp::HtmlString & setdiv){
            setdiv << "<div><span>Edit media library</span><br>"
                   << "</div>";
        }

        void Settings(libhttppp::HttpRequest * req, libhtmlpp::HtmlString & setdiv){
            char url[512];

            std::string suburl;

            suburl=req->getRequestURL()+strlen(Args->config->buildurl("settings/media/",url,512));

            size_t delimter_url=suburl.find("/");

            if(delimter_url!=std::string::npos){
                suburl=suburl.substr(0,delimter_url);
            }

            if(suburl=="editalbum"){
                editAlbum(req,setdiv);
                return;
            }

            setdiv << "<div><span>media library</span><br><table>";
            blogi::SQL sql;
            blogi::DBResult res;

            sql << "SELECT media_albums.id,media_albums.name,media_albums.owner,media_albums.created,users.username"
                << " FROM media_albums LEFT JOIN users ON owner=users.id";

            setdiv << "<tr><th>Albumname</th><th>Owner</th><th>Created</th><th>Actions</th></tr>";
            int n = Args->database->exec(&sql,res);
            for(int i=0; i <n; ++i){
                setdiv << "<tr><td>"<< res[i][1] <<"</td><td>" << res[i][4] << "</td><td>" << res[i][3] <<"</td><td>"
                       << "<a href=\""<< Args->config->buildurl("settings/media/delalbum?",url,512) << "albumid" << res[i][0] <<"\">Remove</a>"
                       << " <a href=\""<< Args->config->buildurl("settings/media/editalbum?",url,512) << "albumid=" << res[i][0] <<"\">Edit</a>"
                       << "</td></tr>";
            }
            setdiv << "</table><br>";
            newPage(req,setdiv);
            setdiv << "</div>";
        }

        void initPlugin(){
            Args->edit->addIcon(icondata,icondatalen,"selimage","webp","Insert Image from media albums");
        }

        bool Controller(netplus::con *curcon,libhttppp::HttpRequest *req,libhtmlpp::HtmlElement page){
            return false;
        }
    };
};

extern "C" blogi::PluginApi* create() {
    return new blogi::Media();
}

extern "C" void destroy(blogi::PluginApi* p) {
    delete p;
}
