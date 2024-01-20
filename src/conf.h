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

#include <string>
#include <vector>

#include <confplus/conf.h>

#pragma once

namespace blogi {
    class Config : public confplus::Config {
    public:
        Config(const char *path);
        ~Config();

        const char* getsiteurl();
        const char* getprefix();

        const char* getConfigPath();

        const char* gettemplate();

        const char* getstartpage();

        const char* buildurl(const char *url,char *buffer,size_t size);

        const char* getplgdir(size_t el);
        size_t      getplgdirs();

        const char* getdbdriver();
        const char* getdbopts();

        const char *getlpdomain();
        const char *getlphost();
        const char *getlpbasedn();
        const char *getlpfilter();

        int         gethttpport();
        const char *gethttpaddr();
        int         gethttpmaxcon();

        const char *getRedisHost();
        int         getRedisPort();
        const char *getRedisPassword();

        const char *getsslcertpath();
        const char *getsslkeypath();
    private:
        std::vector<std::string>  _PlgDir;
        std::string               _DBDriver;
        std::string               _DBConnection;
        std::string               _LDAPDomain;
        std::string               _LDAPHost;
        std::string               _LDAPBase;
        std::string               _LDAPFilter;
        std::string               _HttpHost;
        std::string               _HttpBind;
        int                       _HttpPort;
        int                       _MaxCon;
        std::string               _RedisHost;
        int                       _RedisPort;
        std::string               _RedisPassword;
        std::string               _HttpUrl;
        std::string               _HttpPrefix;
        std::string               _Template;
        std::string               _StartPage;
        std::string               _SSLCertpath;
        std::string               _SSLKeypath;
    };
};
