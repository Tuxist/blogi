/*******************************************************************************
Copyright (c) 2021, Jan Koester jan.koester@gmx.net
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the <organization> nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*******************************************************************************/

#include <string>
#include <vector>

#include <netplus/socket.h>
#include <netplus/connection.h>

#include <httppp/http.h>
#include <htmlpp/html.h>

#include "conf.h"
#include "database.h"
#include "auth.h"

#pragma once

namespace blogi {
    enum TemplateFilesTypes {
        GENERIC=0,
        TEXT=3,
        IMAGE=4,
        JAVASCRIPT=5
    };

    struct TemplateFiles{
        std::string Path;
        std::string Content;
        std::string Ending;
        int         Type;
        bool        Compress;
    };

    struct TemplateConfig{
        std::string Theme;
        Database   *TDatabase;
        Config     *config;
    };

    class Template {
    public:
        Template(TemplateConfig &config);
        ~Template();

        void renderPage(const char *name,libhtmlpp::HtmlPage &page,libhtmlpp::HtmlElement &index);

        bool Controller(netplus::con *curcon,libhttppp::HttpRequest *req);

        void printSite(std::string &output,libhtmlpp::HtmlElement index,const char *crrurl,bool login,const char *meta=nullptr);
    private:
        TemplateConfig             _Config;
        std::vector<TemplateFiles> _PublicFiles;
    };
};
