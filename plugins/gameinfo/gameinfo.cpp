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

#include <cstring>
#include <string>
#include <vector>

#include <gameinfoplus/hldsview.h>

#include <plugin.h>
#include <database.h>
#include <theme.h>
#include <conf.h>
#include <mutex>

#define HIDDEN __attribute__ ((visibility ("hidden")))

namespace blogi {
    HIDDEN std::mutex rlock;

    class GameStatus : public PluginApi {
    public:
        GameStatus(){
        }
        const char* getName(){
            return "gamestatus";
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


            sql << "CREATE TABLE IF NOT EXISTS gameserver_protocols("
                <<   "id integer PRIMARY KEY " << Args->database->autoincrement() << ","
                <<   "pname character varying(255) NOT NULL"
                << "); "
                << "CREATE TABLE IF NOT EXISTS gameserver("
                <<   "id integer PRIMARY KEY " << Args->database->autoincrement() << ","
                <<   "protocol integer,"
                <<   "addr character varying(255) NOT NULL,"
                <<   "port integer,"
                <<   "FOREIGN KEY (protocol) REFERENCES gameserver_protocols (id)"
                << ");";

            Args->database->exec(&sql,res);

            sql="select protocol,addr,port from gameserver";

            int count = Args->database->exec(&sql,res);

            for (int i = 0; i < count; i++) {
                if(atoi(res[i][0])!=0)
                    continue;
                HldsView view(res[i][1],atoi(res[i][2]));
                HLDS.push_back(view);
            }
        }

        bool Controller(libhttppp::HttpRequest *req,libhtmlpp::HtmlElement *page){
            char url[512];
            if(strncmp(req->getRequestURL(),Args->config->buildurl("gamestatus",url,512),strlen(Args->config->buildurl("gamestatus",url,512)))!=0){
                return false;
            }
            libhtmlpp::HtmlString condat;
            libhttppp::HTTPException excep;

            condat << "<div id=\"content\">";

            for (auto i = HLDS.begin(); i!=HLDS.end(); ++i) {
                try{
                    HldsView::HldsData hldsdata;
                    rlock.lock();
                    i->refresh(hldsdata);
                    rlock.unlock();
                    condat << "<div class=\"gameserver\" ><table>";
                    condat << "<tr><td>Gameport: </td><td> " << hldsdata.Port << "</td></tr>";
                    condat << "<tr><td>Gamename: </td><td> " << hldsdata.GameName << "</td></tr>";
                    condat << "<tr><td>Servername: </td><td>" << hldsdata.ServerName << "</td></tr>";
                    condat << "<tr><td>Modname: </td><td>" << hldsdata.ModName << "</td></tr>";
                    condat << "<tr><td>Mapname: </td><td>" << hldsdata.MapName << "</td></tr>";
                    condat << "<tr><td>pwprotected: </td><td>";
                    if(hldsdata.pwProtected)
                        condat << "yes";
                    else
                        condat << "no";
                    condat << "</td></tr>";
                    condat << "<tr><td>Players: </td><td>" << hldsdata.Players  << "("
                    << hldsdata.BotsAmount << ")" << "/"
                    << hldsdata.MaxPlayers
                    << "</td></tr></table></div>";
                }catch(const char *e){
                    std::cerr << e <<std::endl;
                    rlock.unlock();
                }
            }

            condat << "</div>";

            libhtmlpp::HtmlString out;
            std::string sid;
            page->getElementbyID("main")->insertChild(condat.parse());

            Args->theme->printSite(out,page,req->getRequestURL(),
                                    Args->auth->isLoggedIn(req,sid));

            libhttppp::HttpResponse resp;
            resp.setVersion(HTTPVERSION(1.1));
            resp.setState(HTTP200);
            resp.setContentType("text/html");
            resp.send(req,out.c_str(),out.size());
            return true;

        }
    private:
        std::vector<HldsView> HLDS;
    };
};

extern "C" blogi::PluginApi* create() {
    return new blogi::GameStatus();
}

extern "C" void destroy(blogi::PluginApi* p) {
    delete p;
}
