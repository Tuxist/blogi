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

#include <map>
#include <uuid/uuid.h>

#include <htmlpp/html.h>
#include <httppp/http.h>

#include <plugin.h>

#include "icon.webp.h"
#include "types.h"
#include "backend.h"

namespace blogi {
    class Media : public PluginApi {
    public:
        Media(){
            _store=nullptr;
        }

        ~Media(){
            delete _store;
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

        void newAlbum(const int tid,libhttppp::HttpRequest * req, libhtmlpp::HtmlString & setdiv){
            int id = -1;
            std::string albumname;
            blogi::SQL sql,sql2;
            blogi::DBResult res,res2;

            sql << "select id,username from users";

            libhttppp::HttpForm form;

            form.parse(req);

            for(libhttppp::HttpForm::UrlcodedForm::Data *curdat=form.UrlFormData.getFormData(); curdat; curdat=curdat->nextData()){
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
                Args->database[tid]->exec(&sql2,res2);
            }

            setdiv << "<form method=\"POST\" ><span>Albumname</span>:<input name=\"albumname\" type=\"text\" />"
                   << "<select name=\"userid\" >";
            int n = Args->database[tid]->exec(&sql,res);
            for(int i=0; i <n; ++i){
                setdiv << "<option value=\"" << res[i][0] <<"\">" << res[i][1] << "</option>";
            }
            setdiv << "</select>"
                   << "<input value=\"create\" type=\"submit\" /></form>";
        }

        void editAlbum(const int tid,libhttppp::HttpRequest * req, libhtmlpp::HtmlString & setdiv){
            int id = -1;
            char url[512];
            libhttppp::HttpForm form;
            form.parse(req);
            std::string name;

            for(libhttppp::HttpForm::UrlcodedForm::Data *curdat=form.UrlFormData.getFormData(); curdat; curdat=curdat->nextData()){
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
                Args->database[tid]->exec(&sql,res);
                sql.clear();
            }

            sql << "SELECT name FROM media_albums WHERE id='" << id <<"' LIMIT 1";

            if(Args->database[tid]->exec(&sql,res)<0){
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

        void uploadMedia(const int tid,int id,libhttppp::HttpRequest * req, libhtmlpp::HtmlString & setdiv){
            char url[512];
            std::map<std::vector<char>,std::vector<char>> media;
            libhttppp::HttpForm form;

            form.parse(req);

            for (libhttppp::HttpForm::MultipartForm::Data *curformdat = form.MultipartFormData.getFormData(); curformdat; curformdat = curformdat->nextData()) {
                for(libhttppp::HttpForm::MultipartForm::Data::ContentDisposition* curctdisp = curformdat->getDisposition(); curctdisp;
                    curctdisp=curctdisp->nextContentDisposition()){
                    std::vector<char> name;
                if(strcmp(curctdisp->getKey(),"filename")==0){
                    std::copy(curctdisp->getValue(),curctdisp->getValue()+strlen(curctdisp->getValue()),
                              std::inserter<std::vector<char>>(name,name.begin()));
                    name.push_back('\0');
                    media.insert({name,curformdat->Value});
                }
                    }
            }

            for(const auto& [mediafilename, mediafile] : media){
                std::string ext;
                size_t expos=std::string::npos;

                for(size_t im=mediafilename.size()-1; im > 0; --im){
                    if(mediafilename[im]=='.')
                        expos=im;
                }

                if(expos==std::string::npos){
                    libhttppp::HTTPException excep;
                    excep[libhttppp::HTTPException::Error] << "Upload media Wrong filename!";
                    throw excep;
                }

                if( (++expos) > mediafilename.size()){
                    libhttppp::HTTPException excep;
                    excep[libhttppp::HTTPException::Error] << "Upload media Wrong filename!";
                    throw excep;
                }

                std::copy(mediafilename.begin()+expos,mediafilename.end(),
                          std::inserter<std::string>(ext,ext.begin()));

                blogi::SQL sql;
                blogi::DBResult res;

                sql << "SELECT id FROM media_type where ext='"; sql.escaped(ext.c_str()) <<"' LIMIT 1";

                int n = Args->database[tid]->exec(&sql,res);

                if(n<1){
                    libhttppp::HTTPException excep;
                    excep[libhttppp::HTTPException::Error] << "media type: " << ext.c_str() << " not found in Database aborting!";
                    throw excep;
                }

                int tid=atoi(res[0][0]);

                sql.clear();

                std::string pname;
                std::copy(mediafilename.begin(),mediafilename.begin()+expos,
                          std::inserter<std::string>(pname,pname.begin()));


                sql << "INSERT INTO media_items (album_id,name) VALUES('" << id << "','"; sql.escaped(pname.c_str()) << "') RETURNING id";

                Args->database[tid]->exec(&sql,res);
                sql.clear();

                int mid=atoi(res[0][0]);

                uuid_t fuuid;
                char   cfuuid[512];

                uuid_generate(fuuid);
                uuid_unparse(fuuid,cfuuid);
                uuid_clear(fuuid);

                sql << "INSERT INTO media_items_files (media_items_id,redis_uuid,media_type_id,public) VALUES('"
                << mid << "','" << cfuuid << "','" << tid << "',True)";

                Args->database[tid]->exec(&sql,res);
                sql.clear();
                _store->save(tid,cfuuid,mediafile.data(),mediafile.size());

            }


            blogi::SQL sql,sql2;
            blogi::DBResult res,res2;

            sql << "SELECT id FROM media_items WHERE album_id='" << id << "'";

            int n = Args->database[tid]->exec(&sql,res);

            setdiv << "<div id=\"viewmedia\"><ul>";

            for(int i=0; i<n; ++i){
                sql2 << "SELECT redis_uuid,media_type.ext,media_type.type,media_type.ctype FROM media_items_files LEFT JOIN media_type ON"
                << " media_items_files.media_type_id=media_type.id WHERE media_items_id='" << res[i][0] << "'";
                int nn = Args->database[tid]->exec(&sql2,res2);
                for(int ii=0; ii<nn; ++ii){
                    if(atoi(res2[ii][2])==blogi::MediaTypes::Picture)
                        setdiv << "<li class=\"upreview\" ><a><img src=\"" << Args->config->buildurl("media/getimage/",url,512) << res2[ii][0] << "." << res2[ii][1] << "\"></a></li>";
                    else if(atoi(res2[ii][2])==blogi::MediaTypes::Video)
                        setdiv<< "<li class=\"upreview\" ><a><video> <source src=\"" << Args->config->buildurl("media/getimage/",url,512) << res2[ii][0] << "." << res2[ii][1] << "\""
                        << " type=\"" <<  res2[ii][3] << "\"></video></a></li>";
                }
                sql2.clear();
            }

            setdiv << "</ul></div><br>";

            setdiv << "<div><span>Upload Media:</span><br>"
            << "<form method=\"POST\" enctype=\"multipart/form-data\" >"
            << "<input style=\"display:none;\" type=\"text\" name=\"albumid\" value=\""<< id <<"\"/>"
            << "<input type=\"file\" name=\"mediafile\" />"
            << "<input type=\"submit\" value=\"upload\"/></form>"
            << "</div>";
        }

        void viewAlbum(const int tid,libhttppp::HttpRequest * req, libhtmlpp::HtmlString & setdiv){
            int id;
            libhttppp::HttpForm form;
            form.parse(req);

            for(libhttppp::HttpForm::UrlcodedForm::Data *curdat=form.UrlFormData.getFormData(); curdat; curdat=curdat->nextData()){
                if(strcmp(curdat->getKey(),"albumid")==0)
                    id=atoi(curdat->getValue());
            }

            for (libhttppp::HttpForm::MultipartForm::Data *curformdat = form.MultipartFormData.getFormData(); curformdat; curformdat = curformdat->nextData()) {
                for(libhttppp::HttpForm::MultipartForm::Data::ContentDisposition* curctdisp = curformdat->getDisposition();
                    curctdisp; curctdisp=curctdisp->nextContentDisposition()){
                    if(strcmp(curctdisp->getValue(),"albumid")==0){
                        for(size_t i =0; i<curformdat->Value.size(); ++i){
                            if(!isdigit(curformdat->Value[i])){
                            libhttppp::HTTPException excep;
                            excep[libhttppp::HTTPException::Error] << "Wrong formated Pageid!";
                            throw excep;
                            }
                        }
                        std::vector<char> buf;
                        std::copy(curformdat->Value.begin(),curformdat->Value.end(),
                                  std::inserter<std::vector<char>>(buf,buf.begin()));
                        buf.push_back('\0');
                        id=atoi(buf.data());
                    }
                }
            }

            if(id<0){
                libhttppp::HTTPException exp;
                exp[libhttppp::HTTPException::Warning] << "no album id found!";
                throw exp;
            }

            setdiv << "<div><span>View media library</span><br>";
            uploadMedia(tid,id,req,setdiv);
            setdiv << "</div>";
        }

        void editMediaTypes(const int tid,libhttppp::HttpRequest * req, libhtmlpp::HtmlString & setdiv){
            char url[512];

            libhttppp::HttpForm form;
            int mtype=-1,mid=-1;
            bool confirmed=false;
            std::string mfext,mctype;

            form.parse(req);

            for(libhttppp::HttpForm::UrlcodedForm::Data *curdat=form.UrlFormData.getFormData(); curdat; curdat=curdat->nextData()){
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

                        Args->database[tid]->exec(&sql,res);
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

                Args->database[tid]->exec(&sql,res);
            }


            setdiv << "<div><span>Edit media types </span>";
            blogi::SQL sql;
            blogi::DBResult res;

            sql << "SELECT id,type,ext,ctype FROM media_type";

            setdiv << "<table><tr><th>Type</th><th>File extension</th><th>Contentype</th><th>Actions</th></tr>";

            int n = Args->database[tid]->exec(&sql,res);
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

        void Settings(const int tid,libhttppp::HttpRequest * req, libhtmlpp::HtmlString & setdiv){
            char url[512];

            std::string suburl;

            suburl=req->getRequestURL()+strlen(Args->config->buildurl("settings/media/",url,512));

            size_t delimter_url=suburl.find("/");

            if(delimter_url!=std::string::npos){
                suburl=suburl.substr(0,delimter_url);
            }

            if(suburl=="editalbum"){
                editAlbum(tid,req,setdiv);
                return;
            }else if(suburl=="editmediatypes"){
                editMediaTypes(tid,req,setdiv);
                return;
            }else if(suburl=="viewalbum"){
                viewAlbum(tid,req,setdiv);
                return;
            }


            setdiv << "<div><span>media library</span><br>"
                      "<a href=\""<< Args->config->buildurl("settings/media/editmediatypes",url,512) <<"\" >Edit Media Types</a><br><table>";
            blogi::SQL sql;
            blogi::DBResult res;

            sql << "SELECT media_albums.id,media_albums.name,media_albums.owner,media_albums.created,users.username"
                << " FROM media_albums LEFT JOIN users ON owner=users.id";

            setdiv << "<tr><th>Albumname</th><th>Owner</th><th>Created</th><th>Actions</th></tr>";
            int n = Args->database[tid]->exec(&sql,res);
            for(int i=0; i <n; ++i){
                setdiv << "<tr><td>"<< res[i][1] <<"</td><td>" << res[i][4] << "</td><td>" << res[i][3] <<"</td><td>"
                       << "<a href=\""<< Args->config->buildurl("settings/media/delalbum?",url,512) << "albumid=" << res[i][0] <<"\">Remove</a>"
                       << " <a href=\""<< Args->config->buildurl("settings/media/editalbum?",url,512) << "albumid=" << res[i][0] <<"\">Edit</a>"
                       << " <a href=\""<< Args->config->buildurl("settings/media/viewalbum?",url,512) << "albumid=" << res[i][0] <<"\">View</a>"
                       << "</td></tr>";
            }
            setdiv << "</table><br>";
            newAlbum(tid,req,setdiv);
            setdiv << "</div>";
        }

        void initPlugin(){

            blogi::SQL sql;
            blogi::DBResult res;
            sql << "CREATE TABLE IF NOT EXISTS media_albums ("
                <<   "id integer PRIMARY KEY " << Args->database[0]->autoincrement() << ","
                <<   "name character varying(255) NOT NULL,"
                <<   "owner integer NOT NULL,"
                <<   "created date NOT NULL,"
                <<   "FOREIGN KEY (owner) REFERENCES users (id)"
                << "); "
                << "CREATE TABLE IF NOT EXISTS media_type ("
                <<   "id integer PRIMARY KEY " << Args->database[0]->autoincrement() << ","
                <<   "type integer NOT NULL,"
                <<   "ext character varying(255) NOT NULL,"
                <<   "ctype character varying(255) NOT NULL"
                << "); "
                << "CREATE TABLE IF NOT EXISTS media_items ("
                <<   "id integer PRIMARY KEY " << Args->database[0]->autoincrement() << ","
                <<   "album_id integer NOT NULL,"
                <<   "name character varying(255) NOT NULL,"
                <<   "FOREIGN KEY (album_id) REFERENCES media_albums (id)"
                << "); "
                << "CREATE TABLE IF NOT EXISTS media_items_files ("
                <<   "id integer PRIMARY KEY " << Args->database[0]->autoincrement() << ","
                <<   "media_items_id integer NOT NULL,"
                <<   "redis_uuid uuid NOT NULL,"
                <<   "media_type_id integer NOT NULL,"
                <<   "public boolean NOT NULL,"
                <<   "FOREIGN KEY (media_items_id) REFERENCES media_items (id),"
                <<   "FOREIGN KEY (media_type_id) REFERENCES media_type (id)"
                << "); "
                << "CREATE TABLE IF NOT EXISTS media_preview ("
                <<   "id integer PRIMARY KEY " << Args->database[0]->autoincrement() << ","
                <<   "name character varying(255) NOT NULL,"
                <<   "options character varying(255),"
                <<   "media_type_id integer,"
                <<   "FOREIGN KEY (media_type_id) REFERENCES media_type (id)"
                << "); "
                << "CREATE TABLE IF NOT EXISTS media_items_preview ("
                <<   "id integer PRIMARY KEY " << Args->database[0]->autoincrement() << ","
                <<   "media_items_id integer NOT NULL,"
                <<   "redis_uuid uuid NOT NULL,"
                <<   "media_type_id integer NOT NULL,"
                <<   "public boolean NOT NULL,"
                <<   "FOREIGN KEY (media_items_id) REFERENCES media_items (id),"
                <<   "FOREIGN KEY (media_type_id) REFERENCES media_type (id)"
                << ");";
            Args->database[0]->exec(&sql,res);

            Args->edit->addIcon(icondata,icondatalen,"selimage","webp","Insert Image from media albums");

            if(Args->config->getRedisPassword()){
                _store = new RedisStore(Args->config->getRedisHost(),Args->config->getRedisPort(),
                                        Args->config->getRedisPassword(),Args->config->getRedisTimeout(),Args->maxthreads);
            }else{
                _store = new RedisStore(Args->config->getRedisHost(),Args->config->getRedisPort(),
                                        nullptr,Args->config->getRedisTimeout(),Args->maxthreads);
            }
        }

        bool Controller(const int tid,libhttppp::HttpRequest *req,libhtmlpp::HtmlElement *page){
            char url[512];
            const char *ccurl=req->getRequestURL();

            if (strncmp(ccurl,Args->config->buildurl("media/getimage/",url,512),strlen(Args->config->buildurl("media/getimage",url,512)))==0){
                size_t plen=strlen(Args->config->buildurl("media/getimage/",url,512));
                size_t mlen=strlen(ccurl);

                if( int(mlen-plen) <0)
                    return false;

                std::vector<char> suuid;

                _getSuuid(ccurl,plen,suuid);

                blogi::SQL sql;
                blogi::DBResult res;

                sql << "SELECT media_type.ctype FROM media_items_files LEFT JOIN media_type ON media_items_files.media_Type_id=media_type.id WHERE media_items_files.redis_uuid='";
                sql.escaped(suuid.data()) <<"'";

                int n = Args->database[tid]->exec(&sql,res);

                libhttppp::HttpResponse curres;

                if(n>0){
                    req->RecvData.pos=0;
                    curres.setState(HTTP200);
                    curres.setContentType(res[0][0]);
                    curres.setContentLength(_store->getSize(tid,suuid.data()));
                    curres.send(req,nullptr,-1);
                }else{
                    std::cout << "test 404" << std::endl;
                    curres.setVersion(HTTPVERSION(1.1));
                    curres.setState(HTTP404);
                    curres.send(req,nullptr,0);
                }

                return true;
            }
            return false;
        }

        bool Response(const int tid,libhttppp::HttpRequest * req){
            char url[512];
            const char *ccurl=req->getRequestURL();

            if (strncmp(ccurl,Args->config->buildurl("media/getimage/",url,512),strlen(Args->config->buildurl("media/getimage",url,512)))==0){
                size_t plen=strlen(Args->config->buildurl("media/getimage/",url,512));

                std::vector<char> suuid,data;
                _getSuuid(ccurl,plen,suuid);


                size_t msize = _store->getSize(tid,suuid.data());

                size_t ssize = msize-req->SendData.pos < BLOCKSIZE ? msize-req->SendData.pos : BLOCKSIZE;

                if(req->RecvData.pos<msize){
                    _store->load(tid,req,suuid.data(),data,req->RecvData.pos,ssize);
                    req->SendData.append(data.data(),data.size());
                    req->RecvData.pos+=data.size();
                    return true;
                }
            }
            return false;
        }

    private:
        void _getSuuid(const char *rurl,size_t plen,std::vector<char> &suuid){
            int mlen=strlen(rurl);
            size_t tpos=std::string::npos;

            for(size_t i = mlen; i>plen; --i){
                if(rurl[i]=='.'){
                    tpos=i;
                    break;
                }
            }

            if(tpos==std::string::npos){
                libhttppp::HTTPException e;
                e[libhttppp::HTTPException::Error] << "blogi media plugin: no valid media url!";
                throw e;
            }

            std::copy(rurl+plen,rurl+tpos,std::inserter<std::vector<char>>(suuid,suuid.begin()));

            suuid.push_back('\0');

        }
        Store *_store;
    };
};

extern "C" blogi::PluginApi* create() {
    return new blogi::Media();
}

extern "C" void destroy(blogi::PluginApi* p) {
    delete p;
}
