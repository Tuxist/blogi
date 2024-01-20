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

#include <httppp/http.h>

#include "database.h"
#include "session.h"
#include "conf.h"

#pragma once

extern "C" {
    #include <stdio.h>
#ifdef LDAPSUPPORT
    #include <ldap.h>
#endif
}

namespace blogi {

    class Auth{
    public:
        Auth(blogi::Database *pcon,blogi::Session *session,blogi::Config *cfg);
        ~Auth();
        bool login(const char *username,const char *password,std::string &ssid);
        bool isLoggedIn(libhttppp::HttpRequest *curreq,std::string &sessionid);
    private:
#ifdef LDAPSUPPORT
        bool ldapLogin(const char *username,const char *password,std::string &ssid);
#endif
        bool locallogin(const char *username,const char *password,std::string &ssid);

        blogi::Database *_dbconn;
        blogi::Session  *_session;
        blogi::Config   *_config;
#ifdef LDAPSUPPORT
        LDAPControl    *_serverctrls;
        LDAPControl    *_clientctrls;
#endif
    };
};
