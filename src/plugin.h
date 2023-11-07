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

#include <httppp/exception.h>
#include <httppp/http.h>

#include "conf.h"
#include "database.h"
#include "session.h"
#include "auth.h"
#include "theme.h"

#pragma once

namespace blogi {
    struct PluginArgs{
        Database *database;
        Session  *session;
        Auth     *auth;
        Template *theme;
        Config   *config;
    };

    class PluginApi {
    public:
        virtual ~PluginApi()=0;
        virtual const char* getName()=0;
        virtual const char* getVersion()=0;
        virtual const char* getAuthor()=0;
        virtual bool        haveSettings();

        void setArgs(PluginArgs *args);

        virtual void initPlugin()=0;

        virtual bool Controller(netplus::con *curcon,libhttppp::HttpRequest *req,libhtmlpp::HtmlElement page);

        virtual void Settings(libhttppp::HttpRequest *req,libhtmlpp::HtmlString &setdiv);

        virtual void Rendering(libhttppp::HttpRequest *req,libhtmlpp::HtmlElement &curpage);
    protected:
        PluginArgs *Args;
    };

    typedef PluginApi* create_t();
    typedef void destroy_t(PluginApi*);

    class Plugin {
    public:
        Plugin();
        ~Plugin();

        void loadPlugins(const char *path,PluginArgs *args);

        class PluginData{
        private:
            PluginData();
            ~PluginData();
            void       *pldata;
            PluginApi  *ins;
            PluginData *next;
        public:
            PluginApi  *getInstace();
            PluginData *getNextPlg();
            friend class Plugin;
       };

       PluginData *getFirstPlugin();

    private:
       PluginData *_firstPlugin;
       PluginData *_lastPluging;
   };
};
