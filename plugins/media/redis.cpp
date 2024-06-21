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

#include <iostream>
#include <algorithm>
#include <cstring>

#include <httppp/exception.h>
#include <httppp/httpd.h>

#include "backend.h"

blogi::RedisStore::RedisStore(const char *host,int port,const char *password,int millitout){

    struct timeval timeout = { 0, millitout }; // 1.5 seconds

    _RedisCTX=redisConnectWithTimeout(host,port,timeout);

    if (_RedisCTX->err) {
        libhttppp::HTTPException exp;
        exp[libhttppp::HTTPException::Error] << "media plugin err: " << _RedisCTX->errstr;
        throw exp;
    }

    if(password){
        _RedisPassword=password;
        redisCommand(_RedisCTX,"AUTH %s", password);
    }

}
blogi::RedisStore::~RedisStore(){
    redisFree(_RedisCTX);
}

size_t blogi::RedisStore::getSize(const char* key){
        redisReply *rep=(redisReply*)redisCommand(_RedisCTX,"STRLEN %s",key);

        if(_RedisCTX->err!=REDIS_OK){
            libhttppp::HTTPException e;
            e[libhttppp::HTTPException::Error] << "media plugin err: " << _RedisCTX->errstr;
            throw e;
        }

        return rep->integer;
}


void blogi::RedisStore::save(const char *key, const char *data,size_t datalen){
    redisCommand(_RedisCTX,"SET %s %b",key,data,datalen);
    redisCommand(_RedisCTX,"save");
}

void blogi::RedisStore::load(libhttppp::HttpRequest *req,const char *key,std::vector<char> &data,size_t pos,size_t blocksize) {
    try{
        redisReply *rep=(redisReply*)redisCommand(_RedisCTX,"GETRANGE %s %d %d",key,pos,pos+blocksize);

        if(_RedisCTX->err!=REDIS_OK){
            libhttppp::HTTPException e;
            e[libhttppp::HTTPException::Error] << "media plugin err: " << _RedisCTX->errstr;
            throw e;
        }

        std::copy(rep->str,rep->str+rep->len,std::inserter<std::vector<char>>(data,data.begin()));

        freeReplyObject(rep);
    }catch(libhttppp::HTTPException &e){
        throw e;
    }
}
