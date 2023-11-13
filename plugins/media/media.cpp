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

#include <uuid/uuid.h>
#include <hiredis/hiredis.h>

#include <htmlpp/html.h>
#include <httppp/http.h>

#include <plugin.h>

#include "icon.webp.h"
#include "types.h"

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

        void newAlbum(libhttppp::HttpRequest * req, libhtmlpp::HtmlString & setdiv){
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

            if(id >=0 && !albumname.empty()){
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
            int id = -1;
            char url[512];
            libhttppp::HttpForm form;
            form.parse(req);
            std::string name;

            for(libhttppp::HttpForm::UrlcodedFormData *curdat=form.getUrlcodedFormData(); curdat; curdat=curdat->nextUrlcodedFormData()){
                if(strcmp(curdat->getKey(),"albumid")==0)
                    id=atoi(curdat->getValue());
                if(strcmp(curdat->getKey(),"albumname")==0)
                    libhtmlpp::HtmlEncode(curdat->getValue(),name);
            }

            if(id<0){
                libhttppp::HTTPException exp;
                exp[libhttppp::HTTPException::Warning] << "no album id for editing!";
                throw exp;
            }

            blogi::SQL sql;
            blogi::DBResult res;

            if(!name.empty()){
                sql << "UPDATE media_albums set name='"; sql.escaped(name.c_str()) <<"' WHERE id='" << id << "'";
                Args->database->exec(&sql,res);
                sql.clear();
            }

            sql << "SELECT name FROM media_albums WHERE id='" << id <<"' LIMIT 1";

            if(Args->database->exec(&sql,res)<0){
                libhttppp::HTTPException exp;
                exp[libhttppp::HTTPException::Warning] << "this album id is not found Database";
                throw exp;
            }

            name=res[0][0];

            sql.clear();

            setdiv << "<div><span>Edit media library</span><br>"
                   << "<form method=\"POST\">"
                   << "<input style=\"display:none;\" type=\"text\" name=\"albumid\" value=\""<< id <<" \"/>"
                   << "<input type=\"text\" name=\"albumname\" value=\""<< name <<" \"/>"
                   << "<input type=\"submit\" value=\"save\"/></form>"
                   << "</div>";
        }

        void uploadMedia(int id,libhttppp::HttpForm::MultipartFormData *dat,libhttppp::HttpRequest * req, libhtmlpp::HtmlString & setdiv){
            std::string mediafile,mediafilename;

            if(dat){
                for (libhttppp::HttpForm::MultipartFormData *curformdat = dat; curformdat; curformdat = curformdat->nextMultipartFormData()) {
                    libhttppp::HttpForm::MultipartFormData::ContentDisposition* curctdisp = curformdat->getContentDisposition();
                    if(curctdisp && strcmp(curctdisp->getName(),"mediafile")==0){
                        mediafilename=curctdisp->getFilename();
                        mediafile.resize(curformdat->getDataSize());
                        std::copy(curformdat->getData(),curformdat->getData()+curformdat->getDataSize(),mediafile.begin());
                    }
                }

                if(!mediafile.empty() && !mediafilename.empty()){
                    std::string ext;
                    size_t expos=mediafilename.rfind('.');
                    if(expos==std::string::npos){
                        libhttppp::HTTPException excep;
                        excep[libhttppp::HTTPException::Error] << "Upload media Wrong filename!";
                        throw excep;
                    }

                    if( (++expos) > mediafilename.length()){
                        libhttppp::HTTPException excep;
                        excep[libhttppp::HTTPException::Error] << "Upload media Wrong filename!";
                        throw excep;
                    }

                    ext=mediafilename.substr(expos,mediafilename.length()-expos);

                    blogi::SQL sql;
                    blogi::DBResult res;

                    sql << "SELECT id FROM media_type where ext='"; sql.escaped(ext.c_str()) <<"' LIMIT 1";

                    int n = Args->database->exec(&sql,res);

                    if(n<1){
                        libhttppp::HTTPException excep;
                        excep[libhttppp::HTTPException::Error] << "media type: " << ext.c_str() << " not found in Database aborting!";
                        throw excep;
                    }

                    int tid=atoi(res[0][0]);

                    sql.clear();

                    sql << "INSERT INTO media_items (album_id,name) VALUES('" << id << "','"; sql.escaped(mediafilename.substr(0,expos).c_str()) << "') RETURNING id";

                    Args->database->exec(&sql,res);
                    sql.clear();

                    int mid=atoi(res[0][0]);

                    uuid_t fuuid;
                    char   cfuuid[512];

                    uuid_generate(fuuid);
                    uuid_unparse(fuuid,cfuuid);
                    uuid_clear(fuuid);

                    sql << "INSERT INTO media_items_files (media_items_id,redis_uuid,media_type_id) VALUES('"
                        << mid << "','" << cfuuid << "','" << tid << "')";

                    Args->database->exec(&sql,res);
                    sql.clear();

                    std::string rediscmd;

                    redisCommand(_RedisCTX,"SET %s %b",cfuuid,
                                 mediafile.c_str(),
                                 mediafile.length());
                    redisCommand(_RedisCTX, "save");
                }

            }

            setdiv << "<div><span>Upload Media:</span><br>"
            << "<form method=\"POST\" enctype=\"multipart/form-data\" >"
            << "<input style=\"display:none;\" type=\"text\" name=\"albumid\" value=\""<< id <<"\"/>"
            << "<input type=\"file\" name=\"mediafile\" />"
            << "<input type=\"submit\" value=\"upload\"/></form>"
            << "</div>";
        }

        void viewAlbum(libhttppp::HttpRequest * req, libhtmlpp::HtmlString & setdiv){
            int id;
            libhttppp::HttpForm form;
            form.parse(req);

            for(libhttppp::HttpForm::UrlcodedFormData *curdat=form.getUrlcodedFormData(); curdat; curdat=curdat->nextUrlcodedFormData()){
                if(strcmp(curdat->getKey(),"albumid")==0)
                    id=atoi(curdat->getValue());
            }

            libhttppp::HttpForm::MultipartFormData *dat=form.getMultipartFormData();

            for (libhttppp::HttpForm::MultipartFormData *curformdat = dat; curformdat; curformdat = curformdat->nextMultipartFormData()) {
                libhttppp::HttpForm::MultipartFormData::ContentDisposition* curctdisp = curformdat->getContentDisposition();
                if(curctdisp && strcmp(curctdisp->getName(),"albumid")==0){
                    for(size_t i =0; i<curformdat->getDataSize(); ++i){
                        if(!isdigit(curformdat->getData()[i])){
                            libhttppp::HTTPException excep;
                            excep[libhttppp::HTTPException::Error] << "Wrong formted Pageid!";
                            throw excep;
                        }
                    }
                    std::string buf;
                    buf.resize(curformdat->getDataSize());
                    std::copy(curformdat->getData(),curformdat->getData()+curformdat->getDataSize(),buf.begin());
                    id=atoi(buf.c_str());
                }
            }

            if(id<0){
                libhttppp::HTTPException exp;
                exp[libhttppp::HTTPException::Warning] << "no album id found!";
                throw exp;
            }

            setdiv << "<div><span>View media library</span><br>";
            uploadMedia(id,dat,req,setdiv);
            setdiv << "</div>";
        }

        void editMediaTypes(libhttppp::HttpRequest * req, libhtmlpp::HtmlString & setdiv){
            char url[512];

            libhttppp::HttpForm form;
            int mtype=-1,mid=-1;
            bool confirmed=false;
            std::string mfext,mctype;

            form.parse(req);

            for(libhttppp::HttpForm::UrlcodedFormData *curdat=form.getUrlcodedFormData(); curdat; curdat=curdat->nextUrlcodedFormData()){
                if(strcmp(curdat->getKey(),"mtype")==0)
                    mtype=atoi(curdat->getValue());
                else if(strcmp(curdat->getKey(),"mfext")==0)
                    mfext=curdat->getValue();
                else if(strcmp(curdat->getKey(),"mctype")==0)
                    mctype=curdat->getValue();
                else if(strcmp(curdat->getKey(),"mtypeid")==0)
                    mid=atoi(curdat->getValue());
                else if(strcmp(curdat->getKey(),"confirmed")==0 && strcmp(curdat->getValue(),"true")==0)
                    confirmed=true;
            }

            if(strncmp(req->getRequestURL()+strlen(Args->config->buildurl("settings/media/editmediatypes/",url,512)),"delmtype",8)==0){
                if(mid>=0){
                    if(confirmed){
                        blogi::SQL sql;
                        blogi::DBResult res;

                        sql << "DELETE FROM media_type WHERE id='" << mid << "'";

                        Args->database->exec(&sql,res);
                    }else{
                        setdiv << "<div><span>Remove media types </span><br>"
                               << "<span>You want Remove this media type ? (All Files oh this type will also Removed !)</span>"
                               << "<a href=\""<< Args->config->buildurl("settings/media/editmediatypes/delmtype?",url,512)
                               << "mtypeid=" << mid << "&confirmed=true\">yes</a>"
                               << "</div>";
                    }
                }
            }

            if(mtype>=0 && !mfext.empty() && !mctype.empty()){
                blogi::SQL sql;
                blogi::DBResult res;

                sql << "INSERT INTO media_type (type,ext,ctype) VALUES ('" << mtype <<"','"; sql.escaped(mfext.c_str()) << "','";  sql.escaped(mctype.c_str()) << "')";

                Args->database->exec(&sql,res);
            }


            setdiv << "<div><span>Edit media types </span>";
            blogi::SQL sql;
            blogi::DBResult res;

            sql << "SELECT id,type,ext,ctype FROM media_type";

            setdiv << "<table><tr><th>Type</th><th>File extension</th><th>Contentype</th><th>Actions</th></tr>";

            int n = Args->database->exec(&sql,res);
            for(int i=0; i <n; ++i){
                setdiv << "<tr><td>"<< res[i][1] <<"</td><td>" << res[i][2] << "</td><td>" << res[i][3] <<"</td><td>"
                       << "<a href=\""<< Args->config->buildurl("settings/media/editmediatypes/delmtype?",url,512) << "mtypeid=" << res[i][0] <<"\">Remove</a>"
                       << "</td></tr>";
            }
            setdiv << "<tr><form method=\"POST\" >"
                   << "<td><select name=\"mtype\" />"
                   << "<option value=\"0\">Picture</option>"
                   << "<option value=\"1\">Audio</option>"
                   << "<option value=\"2\">Video</option>"
                   << "</select></td>"
                   << "<td><input type=\"text\" name=\"mfext\" /></td>"
                   << "<td><input type=\"text\" name=\"mctype\" /></td>"
                   << "<td><input value=\"create\" type=\"submit\" /></td>"
                   <<"</form></tr></table></div>";
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
            }else if(suburl=="editmediatypes"){
                editMediaTypes(req,setdiv);
                return;
            }else if(suburl=="viewalbum"){
                viewAlbum(req,setdiv);
                return;
            }


            setdiv << "<div><span>media library</span><br>"
                      "<a href=\""<< Args->config->buildurl("settings/media/editmediatypes",url,512) <<"\" >Edit Media Types</a><br><table>";
            blogi::SQL sql;
            blogi::DBResult res;

            sql << "SELECT media_albums.id,media_albums.name,media_albums.owner,media_albums.created,users.username"
                << " FROM media_albums LEFT JOIN users ON owner=users.id";

            setdiv << "<tr><th>Albumname</th><th>Owner</th><th>Created</th><th>Actions</th></tr>";
            int n = Args->database->exec(&sql,res);
            for(int i=0; i <n; ++i){
                setdiv << "<tr><td>"<< res[i][1] <<"</td><td>" << res[i][4] << "</td><td>" << res[i][3] <<"</td><td>"
                       << "<a href=\""<< Args->config->buildurl("settings/media/delalbum?",url,512) << "albumid=" << res[i][0] <<"\">Remove</a>"
                       << " <a href=\""<< Args->config->buildurl("settings/media/editalbum?",url,512) << "albumid=" << res[i][0] <<"\">Edit</a>"
                       << " <a href=\""<< Args->config->buildurl("settings/media/viewalbum?",url,512) << "albumid=" << res[i][0] <<"\">View</a>"
                       << "</td></tr>";
            }
            setdiv << "</table><br>";
            newAlbum(req,setdiv);
            setdiv << "</div>";
        }

        void initPlugin(){
            Args->edit->addIcon(icondata,icondatalen,"selimage","webp","Insert Image from media albums");

            _RedisCTX=redisConnect("127.0.0.1", 6381);

            if (_RedisCTX->err) {
                libhttppp::HTTPException exp;
                exp[libhttppp::HTTPException::Warning] << "media plugin err: " << _RedisCTX->errstr;
                throw exp;
            }

        }

        bool Controller(netplus::con *curcon,libhttppp::HttpRequest *req,libhtmlpp::HtmlElement page){
            return false;
        }
    private:
        redisContext *_RedisCTX;
    };
};

extern "C" blogi::PluginApi* create() {
    return new blogi::Media();
}

extern "C" void destroy(blogi::PluginApi* p) {
    delete p;
}
