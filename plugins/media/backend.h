/*******************************************************************************
 * Copyright (c) 2024, Jan Koester jan.koester@gmx.net
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

#pragma once

#include <string>
#include <vector>

#include <hiredis/hiredis.h>

#include <httppp/http.h>

namespace blogi {
    class Store {
    public:
        Store(){};
        virtual ~Store(){};

        virtual size_t getSize(int tid,const char *key)=0;

        virtual void save(int tid,const char *key, const char *data,size_t datalen)=0;
        virtual void load(int tid,libhttppp::HttpRequest *req,const char *key,std::vector<char> &data,size_t pos,size_t blocksize) =0;

    };

    class RedisStore : public Store {
    public:
        RedisStore(const char *host,int port,const char *password,int timeout,int threads=0);
        ~RedisStore();

        size_t getSize(int tid,const char *key) override;

        void save(int tid,const char *key,const char *data,size_t datalen) override;
        void load(int tid,libhttppp::HttpRequest *req,const char *key,std::vector<char> &data,size_t pos,size_t blocksize) override;
    private:
        std::vector<redisContext*> _RedisCTX;
        std::string                _RedisPassword;
        int                        _Threads;
    };
};
