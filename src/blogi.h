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

#include <httppp//httpd.h>

#include "theme.h"
#include "plugin.h"

#pragma once

namespace blogi {
    class Blogi : public libhttppp::HttpEvent  {
    public:
        Blogi(Config *blgcfg,netplus::socket *serversocket);
        ~Blogi();
        void loginPage(libhttppp::HttpRequest *curreq,const int tid);
        void logoutPage(libhttppp::HttpRequest *curreq,const int tid);
        void settingsPage(libhttppp::HttpRequest *curreq,const int tid);
        void RequestEvent(libhttppp::HttpRequest *curreq,const int tid,void *args);
        void ResponseEvent(libhttppp::HttpRequest *curreq,const int tid,void *args);
    private:
        PluginArgs             *PlgArgs;
        libhtmlpp::HtmlPage     Page,MPage;
        libhtmlpp::HtmlElement  Index,MIndex;
        blogi::Config          *BlogConfig;
        blogi::Plugin          *BlogiPlg;
    };
};
