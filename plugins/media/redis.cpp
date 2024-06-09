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

#include <httppp/exception.h>

#include "backend.h"

blogi::RedisStore::RedisStore(const char *host,int port,const char *password){
    _host=host;
    _port=port;
    _RedisCTX=redisConnectNonBlock(host,port);

    if (_RedisCTX->err) {
        libhttppp::HTTPException exp;
        exp[libhttppp::HTTPException::Error] << "media plugin err: " << _RedisCTX->errstr;
        throw exp;
    }

    if(password){
        _pw=password;
        redisReply *reply = (redisReply*)redisCommand(_RedisCTX, "AUTH %s", password);
        if (reply->type == REDIS_REPLY_ERROR) {
            libhttppp::HTTPException exp;
            exp[libhttppp::HTTPException::Error] << "media plugin err: " << _RedisCTX->errstr;
            throw exp;
        }
        freeReplyObject(reply);
    }

}

void blogi::RedisStore::save(const std::string key, const std::vector<char> value){
    libhttppp::HTTPException exp;
    redisReply* reply = (redisReply*) redisCommand(_RedisCTX,"SET %s %b",key.c_str(),value.data(),value.size());
    if (reply && reply->type==REDIS_REPLY_ERROR) {

        _reconnect();

        freeReplyObject(reply);
        libhttppp::HTTPException exp;
        exp[libhttppp::HTTPException::Error] << "media plugin err: " << _RedisCTX->errstr;
        throw exp;
    }
    freeReplyObject(reply);
    reply = (redisReply*) redisCommand(_RedisCTX, "save");
    if (reply && reply->type==REDIS_REPLY_ERROR) {

        _reconnect();

        freeReplyObject(reply);
        libhttppp::HTTPException exp;
        exp[libhttppp::HTTPException::Error] << "media plugin err: " << _RedisCTX->errstr;
        throw exp;
    }
}

void blogi::RedisStore::load(const std::string key,std::vector<char> &value) {
    redisReply* reply = (redisReply*) redisCommand(_RedisCTX, "GET %s",key.c_str());
    if(reply && reply->type!=REDIS_REPLY_ERROR){
        std::copy(reply->str,reply->str+reply->len,std::inserter<std::vector<char>>(value,value.begin()));
    }else{
        freeReplyObject(reply);

        _reconnect();

        libhttppp::HTTPException exp;
        exp[libhttppp::HTTPException::Error] << "media plugin err: " << _RedisCTX->errstr;
        throw exp;
    }
    freeReplyObject(reply);
}

void blogi::RedisStore::_reconnect(){

    redisReconnect(_RedisCTX);

    if (_RedisCTX->err) {
        libhttppp::HTTPException exp;
        exp[libhttppp::HTTPException::Error] << "media plugin err: " << _RedisCTX->errstr;
        throw exp;
    }

    if(!_pw.empty()){
        redisReply *reply = (redisReply*)redisCommand(_RedisCTX, "AUTH %s", _pw.c_str());
        if (reply->type == REDIS_REPLY_ERROR) {
            libhttppp::HTTPException exp;
            exp[libhttppp::HTTPException::Error] << "media plugin err: " << _RedisCTX->errstr;
            throw exp;
        }
        freeReplyObject(reply);
    }

}
