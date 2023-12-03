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

#include <stdio.h>

#include "conf.h"

blogi::Config *blogi::Config::_Instance=nullptr;

blogi::Config::Config(){
    _PlsConfig=nullptr;
}

blogi::Config::~Config(){
    delete _PlsConfig;
}

blogi::Config* blogi::Config::getInstance(){
    if(_Instance==nullptr)
        _Instance=new Config;
    return _Instance;
}

void blogi::Config::loadconfig(const char* path){
    if(_PlsConfig)
        delete _PlsConfig;
    _PlsConfig=new confplus::Config(path);
}


const char * blogi::Config::getsiteurl(){
    if(_PlsConfig->getKey("/BLOGI/HTTP/URL"))
        return _PlsConfig->getValue(_PlsConfig->getKey("/BLOGI/HTTP/URL"),0);
    return nullptr;
}

const char * blogi::Config::getprefix(){
    if(_PlsConfig->getKey("/BLOGI/HTTP/PREFIX"))
        return _PlsConfig->getValue(_PlsConfig->getKey("/BLOGI/HTTP/PREFIX"),0);
    return nullptr;
}

const char * blogi::Config::gettemplate(){
    if(_PlsConfig->getKey("/BLOGI/TEMPLATE"))
        return _PlsConfig->getValue(_PlsConfig->getKey("/BLOGI/TEMPLATE"),0);
    return nullptr;
}

const char * blogi::Config::getstartpage(){
    if(_PlsConfig->getKey("/BLOGI/STARTPAGE"))
        return _PlsConfig->getValue(_PlsConfig->getKey("/BLOGI/STARTPAGE"),0);
    return nullptr;
}

const char * blogi::Config::buildurl(const char *url,char *buffer,size_t size){
    if(!getprefix())
        return nullptr;
    snprintf(buffer,size,"%s/%s",getprefix(),url);
    return buffer;
}

const char *blogi::Config::getplgdir(size_t el){
    if(_PlsConfig->getKey("/BLOGI/PLUGINDIR"))
        return _PlsConfig->getValue(_PlsConfig->getKey("/BLOGI/PLUGINDIR"),el);
    return nullptr;
}

size_t blogi::Config::getplgdirs(){
    if(_PlsConfig->getKey("/BLOGI/PLUGINDIR"))
        return _PlsConfig->getElements(_PlsConfig->getKey("/BLOGI/PLUGINDIR"));
    return 0;
}

const char * blogi::Config::getdbopts(){
    if(_PlsConfig->getKey("/BLOGI/DATABASE/CONNECTION"))
        return _PlsConfig->getValue(_PlsConfig->getKey("/BLOGI/DATABASE/CONNECTION"),0);
    return nullptr;
}

const char * blogi::Config::getlpdomain(){
    if(_PlsConfig->getKey("/BLOGI/LDAP/DOMAIN"))
        return _PlsConfig->getValue(_PlsConfig->getKey("/BLOGI/LDAP/DOMAIN"),0);
    return nullptr;
}

const char *blogi::Config::getlphost(){
    if(_PlsConfig->getKey("/BLOGI/LDAP/HOST"))
        return _PlsConfig->getValue(_PlsConfig->getKey("/BLOGI/LDAP/HOST"),0);
    return nullptr;
}

const char * blogi::Config::getlpbasedn(){
    if(_PlsConfig->getKey("/BLOGI/LDAP/BASEDN"))
        return _PlsConfig->getValue(_PlsConfig->getKey("/BLOGI/LDAP/BASEDN"),0);
    return nullptr;
}

const char * blogi::Config::getlpfilter(){
    if(_PlsConfig->getKey("/BLOGI/LDAP/BASEDN"))
        return _PlsConfig->getValue(_PlsConfig->getKey("/BLOGI/LDAP/LOGINFILTER"),0);
    return nullptr;
}

const char * blogi::Config::gethttpaddr(){
    if(_PlsConfig->getKey("/BLOGI/HTTP/BIND"))
        return _PlsConfig->getValue(_PlsConfig->getKey("/BLOGI/HTTP/BIND"),0);
    return nullptr;
}

int blogi::Config::gethttpport(){
    if(_PlsConfig->getKey("/BLOGI/HTTP/PORT"))
        return _PlsConfig->getIntValue(_PlsConfig->getKey("/BLOGI/HTTP/PORT"),0);
    return -1;
}

int blogi::Config::gethttpmaxcon(){
    if(_PlsConfig->getKey("/BLOGI/HTTP/MAXCON"))
        return _PlsConfig->getIntValue(_PlsConfig->getKey("/BLOGI/HTTP/MAXCON"),0);
    return -1;
}

const char * blogi::Config::getRedisHost(){
    if(_PlsConfig->getKey("/BLOGI/REDIS/HOST"))
        return _PlsConfig->getValue(_PlsConfig->getKey("/BLOGI/REDIS/HOST"),0);
    return nullptr;
}

const char * blogi::Config::getRedisPassword(){
    if(_PlsConfig->getKey("/BLOGI/REDIS/PASSWORD"))
        return _PlsConfig->getValue(_PlsConfig->getKey("/BLOGI/REDIS/PASSWORD"),0);
    return nullptr;
}

int blogi::Config::getRedisPort(){
    if(_PlsConfig->getKey("/BLOGI/REDIS/PORT"))
        return _PlsConfig->getIntValue(_PlsConfig->getKey("/BLOGI/REDIS/PORT"),0);
    return -1;
}


