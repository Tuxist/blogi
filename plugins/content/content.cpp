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

#include<iostream>

#include <htmlpp/html.h>
#include <httppp/http.h>
#include <httppp/exception.h>

#include <database.h>
#include <auth.h>
#include <conf.h>
#include <plugin.h>

#define SITELIMIT 10

namespace blogi {
    class Content : public PluginApi{
    public:
        Content(){
        };

        ~Content(){
        };

        void contentIdxPage(netplus::con *curcon,libhttppp::HttpRequest *curreq,libhtmlpp::HtmlElement &page,const char *tag,int start,int end){
            libhttppp::HTTPException excep;
            libhtmlpp::HtmlString condat;

            char url[512];
            std::string sid;
            blogi::SQL sql;
            blogi::DBResult res;

            int ncount=0;

            if (tag) {
                sql = "select id from tags where name='"; sql.escaped(tag) << "' LIMIT 1";
                if(Args->database->exec(&sql,res)<1){
                    excep[libhttppp::HTTPException::Critical] << "no tag data found for this name!";
                    throw excep;
                }else {
                    sql = "select content.id,content.title,content.descrition,users.username,content.created from content ";
                    sql <<"LEFT JOIN users ON content.author=users.id LEFT JOIN tags_content ON tags_content.content_id=content.id where tags_content.tag_id='"
                    << res[0][0]
                    <<"' ORDER BY id DESC OFFSET '" << start << "' LIMIT " << end;
                    ncount=Args->database->exec(&sql,res);
                }
            } else {
                sql="select content.id,content.title,content.descrition,users.username,content.created from content LEFT JOIN users ON content.author=users.id ORDER BY id DESC";
                sql << " OFFSET '" << start << "' LIMIT " << end;
                ncount=Args->database->exec(&sql,res);
            }

            if (ncount<1) {
                excep[libhttppp::HTTPException::Critical] << "<span> No Content found! </span>";
                throw excep;
            }
            std::string meta;
            condat << "<div id=\"contentidx\">";
            if(Args->auth->isLoggedIn(curreq,sid)){
                condat << "<div class=\"blog_adminmenu\">"
                << "<ul>"
                << "<li><a href=\""<< Args->config->buildurl("content/addpost",url,512) <<"\">Addpost</a></li>"
                << "</ul>"
                << "</div>";
            }
            for (int i = 0; i < ncount; i++) {
                condat << "<div class=\"blog_entry\">"
                << "<span class=\"title\">" << res[i][1] << "</span>"
                << "<div  class=\"entry_text\">" << res[i][2] << "</div>"
                << "<span><a href=\""<< Args->config->buildurl("content/read/",url,512) << res[i][0] << "\">Weiterlesen</a> </span>"
                << "<span class=\"author\">verfasst von " << res[i][3] << " am "<< res[i][4]  <<" </span></div>";
                meta.append(res[i][1]);
                meta.append(" author: ");
                meta.append(res[i][3]);
                meta.append(" date: ");
                meta.append(res[i][4]);
                meta.append(" ");
            }

            condat << "<div id=\"pager\">";

            if((start - end) >= 0)
                condat << "<a href=\"" << curreq->getRequestURL() << "?start=" << (start- end) <<"\" > Zur&uuml;ck </a>";

            if((ncount -10) >= 0 )
                condat << "<a href=\"" << curreq->getRequestURL() << "?start=" << start+10 <<"\" > Weiter </a>";

            condat << " </div> </div>";

            sql="SELECT name,id FROM tags";

            int tcount=Args->database->exec(&sql,res);


            if (tcount>0) {
                condat << "<div id=\"idxtags\"><ul>";

                for (int i = 0; i < tcount; i++) {
                    sql="select from tags_content where tag_id = '";
                    sql << res[i][1] << "'";
                    blogi::DBResult rescnt;
                    condat << "<li><a href=\""
                    << Args->config->buildurl("content/tag/",url,512) << res[i][0] << "\">" << res[i][0] << "(" << Args->database->exec(&sql,rescnt) << ")" << "</a></li>";

                }

                condat << "</ul></div>";
            }
            std::string out;

            page.getElementbyID("main")->insertChild(condat.parse());;

            Args->theme->printSite(out,page,curreq->getRequestURL(),Args->auth->isLoggedIn(curreq,sid),meta.c_str());

            libhttppp::HttpResponse resp;
            resp.setVersion(HTTPVERSION(1.1));
            resp.setState(HTTP200);
            resp.setContentType("text/html");
            resp.send(curcon,out.c_str(),out.length());
        };

        void contentPage(netplus::con* curcon, libhttppp::HttpRequest* curreq,libhtmlpp::HtmlElement &page) {
            libhttppp::HTTPException excep;
            libhtmlpp::HtmlString condat;

            char url[512];
            std::string sid;
            int cid;

            sscanf(curreq->getRequestURL(), Args->config->buildurl("content/read/%d",url,512),&cid);

            blogi::SQL sql;

            sql << "select content.id,content.title,content.text,users.username,content.created from content LEFT JOIN users ON content.author=users.id WHERE content.id='" << cid <<"' LIMIT 1";

            blogi::DBResult res;


            if(Args->database->exec(&sql,res)<1){
                excep[libhttppp::HTTPException::Critical] << "No entry found for content id: " << cid;
                throw excep;
            }

            condat << "<div id=\"content\">";
            if (Args->auth->isLoggedIn(curreq,sid)) {
                condat << "<div class=\"blog_adminmenu\">"
                << "<ul>"
                << "<li><a href=\"" << Args->config->buildurl("content/edit/",url,512) << cid << "\">Bearbeiten</a></li>"
                << "<li><a href=\"" << Args->config->buildurl("content/del/",url,512) << cid << "\">Loeschen</a></li>"
                << "</ul>"
                << "</div>";
            }
            condat << "<div class=\"blog_entry\">"
            << "<span class=\"title\">" << res[0][1] << "</span>"
            << "<div  class=\"text\">" << res[0][2] << "</div>"
            << "<span class=\"author\">verfasst von " << res[0][3] << " am "<< res[0][4]  <<" </span>"
            << "</div></div>";

            std::string out;

            page.getElementbyID("main")->insertChild(condat.parse());

            Args->theme->printSite(out,page,curreq->getRequestURL(),Args->auth->isLoggedIn(curreq,sid));

            libhttppp::HttpResponse resp;
            resp.setVersion(HTTPVERSION(1.1));
            resp.setState(HTTP200);
            resp.setContentType("text/html");
            resp.send(curcon,out.c_str(),out.length());
        };

        void addPostPage(netplus::con *curcon,libhttppp::HttpRequest *curreq,libhtmlpp::HtmlElement &page){
            std::string sid;
            char url[512];

            if(!Args->auth->isLoggedIn(curreq,sid)){
                libhttppp::HTTPException exp;
                exp[libhttppp::HTTPException::Error] << "Please login before!";
                throw exp;
            }


            libhttppp::HTTPException excep;

            libhttppp::HttpForm curform;
            curform.parse(curreq);

            std::string title;
            std::string text;
            std::string tags;
            std::string descrition;

            if(curform.getBoundary()){
                for(libhttppp::HttpForm::MultipartFormData *curformdat=curform.getMultipartFormData(); curformdat; curformdat=curformdat->nextMultipartFormData()){
                    libhttppp::HttpForm::MultipartFormData::ContentDisposition *curctdisp=curformdat->getContentDisposition();
                    if(curctdisp->getName()){
                        if (strcmp(curctdisp->getName(), "blog_title") == 0) {
                            title.append(curformdat->getData(),curformdat->getDataSize());
                        }else if(strcmp(curctdisp->getName(), "blog_descrition") == 0){
                            descrition.append(curformdat->getData(), curformdat->getDataSize());
                        }else if (strcmp(curctdisp->getName(), "blog_tags") == 0) {
                            tags.append(curformdat->getData(), curformdat->getDataSize());
                        }
                        else if(strcmp(curctdisp->getName(),"blog_content")==0){
                            text.append(curformdat->getData(), curformdat->getDataSize());
                        }
                    }
                }

                size_t content_id = -1;

                if(!text.empty() && !title.empty() && !descrition.empty()){

                    blogi::SQL asql,sql;
                    blogi::DBResult ares,res;

                    std::string uid;
                    Args->session->getSessionData(sid.c_str(),"uid",uid);

                    asql << "SELECT id from users WHERE sid='"<< uid.c_str() <<"' LIMIT 1;";

                    if (Args->database->exec(&asql,ares) < 1) {
                        excep[libhttppp::HTTPException::Error] << "User with SID not found!";
                        throw excep;
                    }

                    int author=atoi(ares[0][0]);


                    time_t t = time(NULL);
                    struct tm time = { 0 };
                    char ttmp[26];
                    localtime_r(&t,&time);
                    asctime_r(&time,ttmp);

                    sql << "INSERT INTO content (title,descrition,text,author,created) VALUES ('";
                    sql.escaped(title.c_str()) << "','";
                    sql.escaped(descrition.c_str()) <<"','";
                    sql.escaped(text.c_str()) <<"','"<< author <<"','" << ttmp << "') RETURNING id;";

                    try {
                        Args->database->exec(&sql,res);
                        content_id=atoi(res[0][0]);
                    }catch(libhttppp::HTTPException &e){
                        throw e;
                    }
                }

                size_t tgsstart=0,tgsend=0;
                if (!tags.empty() && content_id != -1) {
                    for (int i = 0; i <= tags.length(); ++i) {
                        if (tags[i] == ' ' || i == tags.length()) {
                            if((i - tgsstart) >0 ){
                                std::string tag= tags.substr(tgsstart,(i-tgsstart));
                                tgsstart = i+1;
                                int tries = 0;
                                TAGNAMECHECK:
                                blogi::SQL sql;
                                blogi::DBResult res;
                                if (tries>5) {
                                    excep[libhttppp::HTTPException::Critical] << "tag doesn't exists and could not created!";
                                    throw excep;
                                }

                                sql = "select id,name from tags where name='";
                                sql.escaped(tag.c_str()) << "' LIMIT 1;";

                                if (Args->database->exec(&sql,res) != 1) {
                                    sql = "insert into tags (name) VALUES ('";
                                    sql.escaped(tag.c_str()) <<"');";
                                    Args->database->exec(&sql,res);
                                    ++tries;
                                    goto TAGNAMECHECK;
                                }

                                sql =  "insert into tags_content (content_id,tag_id) VALUES ('"; sql << content_id <<"','" << res[0][0] <<"');";
                                Args->database->exec(&sql,res);
                            }
                        }
                    }
                }

                char ccurl[512];
                snprintf(ccurl,512,"%s/content/read/%zu",Args->config->getprefix(),content_id);

                libhttppp::HttpResponse resp;
                resp.setVersion(HTTPVERSION(1.1));
                resp.setState(HTTP307);
                resp.setContentType("text/html");
                *resp.setData("Location") << ccurl;
                resp.send(curcon,nullptr,0);
                return;
            }

            libhtmlpp::HtmlString condat;
            condat << "<div id=\"content\">"
            << "<form action=\""<< Args->config->buildurl("content/addpost",url,512) << "\" method=\"post\" enctype=\"multipart/form-data\" >"
            << "<span>Titel:</span><input maxlength=\"255\" name=\"blog_title\" type=\"text\" style=\"width: 100%;\" /><br/>"
            << "<span>Kurzbeschreibung:</span><input maxlength=\"255\" name=\"blog_descrition\" type=\"text\"  style=\"width: 100%;\" /><br/>"
            << "<span>Schlagworte:</span><input name=\"blog_tags\" type=\"text\"  style=\"width: 100%;\" /><br/>"
            << "<span>Text:</span><textarea name=\"blog_content\" type=\"text\" style=\"width: 100%; height:480px;\"> </textarea>"
            << "<input type=\"submit\" value=\"Blog eintrag Posten\"/>"
            << "</form>"
            << "</div>";

            std::string out;

            page.getElementbyID("main")->insertChild(condat.parse());

            Args->theme->printSite(out,page,curreq->getRequestURL(),Args->auth->isLoggedIn(curreq,sid));

            libhttppp::HttpResponse resp;
            resp.setVersion(HTTPVERSION(1.1));
            resp.setState(HTTP200);
            resp.setContentType("text/html");
            resp.send(curcon,out.c_str(),out.length());
        }

        void delPostPage(netplus::con* curcon, libhttppp::HttpRequest* curreq,libhtmlpp::HtmlElement &page) {
            std::string sid;
            char url[512];

            if(!Args->auth->isLoggedIn(curreq,sid)){
                libhttppp::HTTPException exp;
                exp[libhttppp::HTTPException::Error] << "Please login before!";
                throw exp;
            }

            libhttppp::HTTPException excep;

            blogi::DBResult res;
            blogi::SQL      sql;

            int cid = -1;

            sscanf(curreq->getRequestURL(), Args->config->buildurl("content/del/%d",url,512), &cid);

            sql ="DELETE FROM tags_content WHERE content_id='";
            sql << cid << "';" <<" DELETE FROM content WHERE id='"; sql << cid << "';";

            if(Args->database->exec(&sql,res)<0){
                libhttppp::HTTPException exp;
                exp[libhttppp::HTTPException::Error] << "Can'T content sql data!";
                throw exp;
            }

            cleartags();

            libhtmlpp::HtmlString condat;

            condat << "<span>Beitrag geloescht</span>";

            std::string out;

            page.getElementbyID("main")->insertChild(condat.parse());

            Args->theme->printSite(out,page,curreq->getRequestURL(),Args->auth->isLoggedIn(curreq,sid));

            libhttppp::HttpResponse resp;
            resp.setVersion(HTTPVERSION(1.1));
            resp.setState(HTTP200);
            resp.setContentType("text/html");
            resp.send(curcon,out.c_str(),out.length());

        }

        void cleartags() {

            blogi::SQL sql;
            blogi::DBResult res;

            sql="SELECT name,id FROM tags;";

            int n=Args->database->exec(&sql,res);

            libhttppp::HTTPException excep;


            for (int i = 0; i < n; i++) {

                sql="SELECT COUNT(*) FROM tags_content WHERE tag_id='"; sql << res[i][1] << "';";

                blogi::DBResult res2;

                if(Args->database->exec(&sql,res2)<0){
                    libhttppp::HTTPException excep;
                    excep[libhttppp::HTTPException::Critical] << "clear tags count failed this shouldn't happend!";
                    throw excep;
                }

                if( atoi(res2[0][0])>0)
                    continue;

                sql = "DELETE FROM tags WHERE id='"; sql << res[i][1] << "';";

                if(Args->database->exec(&sql,res2)<0){
                    libhttppp::HTTPException excep;
                    excep[libhttppp::HTTPException::Critical] << "clear tags deletd failed this shouldn't happend!";
                    throw excep;
                }

            }
        }

        void editPostPage(netplus::con* curcon, libhttppp::HttpRequest* curreq,libhtmlpp::HtmlElement &page) {
            std::string sid;
            char url[512];
            if (!Args->auth->isLoggedIn(curreq,sid)) {
                libhttppp::HTTPException exp;
                exp[libhttppp::HTTPException::Error] << "Please login before!";
                throw exp;
            }

            libhttppp::HTTPException excep;

            int cid = -1;

            sscanf(curreq->getRequestURL(), Args->config->buildurl("content/edit/%d",url,512), &cid);

            blogi::SQL sql;
            blogi::DBResult res;

            libhttppp::HttpForm curform;
            curform.parse(curreq);

            std::string title;
            std::string text;
            std::string tags;
            std::string descrition;

            if (curform.getBoundary()) {
                for (libhttppp::HttpForm::MultipartFormData* curformdat = curform.getMultipartFormData(); curformdat; curformdat = curformdat->nextMultipartFormData()) {
                    libhttppp::HttpForm::MultipartFormData::ContentDisposition* curctdisp = curformdat->getContentDisposition();
                    if (curctdisp->getName()) {
                        if (strcmp(curctdisp->getName(), "blog_title") == 0) {
                            title.append(curformdat->getData(), curformdat->getDataSize());
                        }
                        else if (strcmp(curctdisp->getName(), "blog_descrition") == 0) {
                            descrition.append(curformdat->getData(), curformdat->getDataSize());
                        }
                        else if (strcmp(curctdisp->getName(), "blog_tags") == 0) {
                            tags.append(curformdat->getData(), curformdat->getDataSize());
                        }
                        else if (strcmp(curctdisp->getName(), "blog_content") == 0) {
                            text.append(curformdat->getData(), curformdat->getDataSize());;
                        }
                    }
                }

                if (!text.empty() && !title.empty() && !descrition.empty()) {
                    blogi::SQL sqltext;
                    sqltext << "UPDATE content SET title='";
                    sql.escaped(title.c_str()) << "',descrition='";
                    sql.escaped(descrition.c_str()) << "',text='";
                    sql.escaped(text.c_str())  << "' where id='" << cid << "';";

                    blogi::DBResult textres;

                    if (Args->database->exec(&sqltext, textres)<0) {
                        excep[libhttppp::HTTPException::Critical] << "can't update updare content with id: " << cid << " !";
                        throw excep;
                    }
                }

                sql <<  "DELETE FROM tags_content WHERE content_id='" << cid <<"';";

                if (Args->database->exec(&sql,res) < 0) {
                    excep[libhttppp::HTTPException::Critical] << "can't delete old tags id from tags_id!";
                    throw excep;
                }

                cleartags();

                size_t tgsstart = 0, tgsend = 0;
                if (!tags.empty() && cid != -1) {
                    for (int i = 0; i <= tags.length(); ++i) {
                        if (tags[i] == ' ' || i == tags.length()) {
                            if ((i - tgsstart) > 0) {
                                std::string tag = tags.substr(tgsstart, (i - tgsstart));
                                tgsstart = i + 1;
                                int tries = 0;

                                TAGNAMECHECK:
                                if (tries > 5) {
                                    excep[libhttppp::HTTPException::Critical] << "tag doesn't exists and could not created!";
                                    throw excep;
                                }
                                sql="select id,name from tags where name='";
                                sql.escaped(tag.c_str()) << "' LIMIT 1;";

                                int tamount;

                                if ((tamount=Args->database->exec(&sql,res)) < 0) {
                                    excep[libhttppp::HTTPException::Critical] << "can't find existing tags !";
                                    throw excep;
                                }

                                if (tamount != 1) {
                                    sql="insert into tags (name) VALUES ('";
                                    sql.escaped(tag.c_str()) << "');";
                                    Args->database->exec(&sql,res);
                                    ++tries;
                                    goto TAGNAMECHECK;
                                }

                                sql= "insert into tags_content (content_id,tag_id) VALUES ('"; sql << cid << "','" << res[0][0] << "');";

                                if (Args->database->exec(&sql,res) < 0) {
                                    excep[libhttppp::HTTPException::Critical] << "can't create link between tag and content !";
                                    throw excep;
                                }

                            }
                        }
                    }
                }
            }

            sql = "SELECT title,descrition,text,tags.name FROM content ";
            sql << "LEFT JOIN tags_content ON tags_content.content_id = content.id "
                << "LEFT JOIN tags ON tags.id = tags_content.tag_id where content.id = '" << cid << "'";

            libhtmlpp::HtmlString condat;

            int ccamount;

            if ((ccamount=Args->database->exec(&sql,res)) < 0) {
                excep[libhttppp::HTTPException::Critical] << "can't find tages for this content id !";
                throw excep;
            }

            if (ccamount > 0) {
                condat << "<div id=\"content\">"
                << "<form action=\"" << Args->config->buildurl("content/edit/",url,512) << cid << "\" method=\"post\" enctype=\"multipart/form-data\" >"
                << "<span>Titel:</span><input maxlength=\"255\" name=\"blog_title\" type=\"text\" style=\"width: 100%;\" value=\"" << res[0][0] << "\"/><br/>"
                << "<span>Kurzbeschreibung:</span><input maxlength=\"255\" name=\"blog_descrition\" type=\"text\"  style=\"width: 100%;\" value=\"" << res[0][1] << "\"/><br/>"
                << "<span>Schlagworte:</span><input name=\"blog_tags\" type=\"text\"  style=\"width: 100%;\" value=\"";
                for (int i = 0; i < ccamount; ++i) {
                    condat << res[i][3];
                    if ((ccamount - 1) != i)
                        condat << " ";
                }
                condat << "\"/><br/>"
                << "<span>Text:</span><textarea name=\"blog_content\" type=\"text\" style=\"width: 100%; height:480px;\" textarea>" << res[0][2] << "</textarea>"
                << "<input type=\"submit\" value=\"Blog eintrag Posten\"/>"
                << "</form>"
                << "</div>";
            };
            std::string out;

            page.getElementbyID("main")->insertChild(condat.parse());

            Args->theme->printSite(out,page,curreq->getRequestURL(),Args->auth->isLoggedIn(curreq,sid));

            libhttppp::HttpResponse resp;
            resp.setVersion(HTTPVERSION(1.1));
            resp.setState(HTTP200);
            resp.setContentType("text/html");
            resp.send(curcon,out.c_str(),out.length());
        }

        const char *getName(){
            return "content";
        }

        const char *getVersion(){
            return "0.2alpha";
        }

        const char *getAuthor(){
            return "Jan Koester";
        }

        void initPlugin(){
            return;
        }

        bool Controller(netplus::con * curcon, libhttppp::HttpRequest * req,libhtmlpp::HtmlElement page){
            char url[512];
            if(strncmp(req->getRequestURL(),Args->config->buildurl("content",url,512),strlen(Args->config->buildurl("content",url,512)))!=0){
                return false;
            }

            std::string curl=req->getRequestURL();

            std::cerr << curl << std::endl;

            if(curl.find('?')>=0){
                curl=curl.substr(0,curl.find('?'));
            }

            if (strncmp(curl.c_str(),Args->config->buildurl("content/addpost",url,512),strlen(Args->config->buildurl("content/addpost",url,512)))==0){
                    addPostPage(curcon,req,page);
            }else if (strncmp(curl.c_str(), Args->config->buildurl("content/read",url,512), strlen(Args->config->buildurl("content/read",url,512))) == 0) {
                    contentPage(curcon,req,page);
            }else if (strncmp(curl.c_str(), Args->config->buildurl("content/edit",url,512), strlen(Args->config->buildurl("content/edit",url,512))) == 0) {
                    editPostPage(curcon,req,page);
            }else if (strncmp(curl.c_str(), Args->config->buildurl("content/del",url,512), strlen(Args->config->buildurl("content/del",url,512))) == 0) {
                    delPostPage(curcon, req,page);
            }else if (strstr(req->getRequestURL(),"robots.txt")){
               const char *robot = "User-agent: Googlebot\r\n\r\nUser-agent: *\r\nAllow: /";
               libhttppp::HttpResponse resp;
               resp.setVersion(HTTPVERSION(1.1));
               resp.setState(HTTP200);
               resp.setContentType("text/plain");
               resp.send(curcon,robot,strlen(robot));
               return true;
            }else{
                int startpos = 0;
                libhttppp::HttpForm start;
                start.parse(req);
                for (libhttppp::HttpForm::UrlcodedFormData* cururlform = start.getUrlcodedFormData(); cururlform;
                    cururlform = cururlform->nextUrlcodedFormData()){
                    if(strcmp(cururlform->getKey(),"start")==0)
                        startpos=atoi(cururlform->getValue());
                }
                std::cerr << startpos << std::endl;
                if (strcmp(curl.c_str(),Args->config->buildurl("content/tag",url,512))>0){
                    size_t len = strlen(Args->config->buildurl("content/tag/",url,512));
                    contentIdxPage(curcon,req,page,curl.substr(len,curl.length()-len).c_str(),startpos,SITELIMIT);
                 }else{
                    contentIdxPage(curcon,req,page,nullptr,startpos,SITELIMIT);
                 }
            }
            return true;
        }
    };
};

extern "C" blogi::PluginApi* create() {
    return new blogi::Content();
}

extern "C" void destroy(blogi::PluginApi* p) {
    delete p;
}
