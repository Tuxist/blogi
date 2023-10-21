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
    return _PlsConfig->getValue(_PlsConfig->getKey("/BLOGI/HTTP/URL"),0);
}


const char * blogi::Config::getprefix(){
    return _PlsConfig->getValue(_PlsConfig->getKey("/BLOGI/HTTP/PREFIX"),0);
}

const char * blogi::Config::gettemplate(){
    return _PlsConfig->getValue(_PlsConfig->getKey("/BLOGI/TEMPLATE"),0);
}

const char * blogi::Config::getstartpage(){
    return _PlsConfig->getValue(_PlsConfig->getKey("/BLOGI/STARTPAGE"),0);
}

const char * blogi::Config::buildurl(const char *url,char *buffer,size_t size){
    snprintf(buffer,size,"%s/%s",getprefix(),url);
    return buffer;
}

const char *blogi::Config::getplgdir(size_t el){
    return _PlsConfig->getValue(_PlsConfig->getKey("/BLOGI/PLUGINDIR"),el);
}

size_t blogi::Config::getplgdirs(){
    return _PlsConfig->getElements(_PlsConfig->getKey("/BLOGI/PLUGINDIR"));
}

const char * blogi::Config::getdbopts(){
    return _PlsConfig->getValue(_PlsConfig->getKey("/BLOGI/DATABASE/CONNECTION"),0);
}

const char * blogi::Config::getlpdomain(){
    return _PlsConfig->getValue(_PlsConfig->getKey("/BLOGI/LDAP/DOMAIN"),0);
}

const char *blogi::Config::getlphost(){
    return _PlsConfig->getValue(_PlsConfig->getKey("/BLOGI/LDAP/HOST"),0);
}

const char * blogi::Config::getlpbasedn(){
    return _PlsConfig->getValue(_PlsConfig->getKey("/BLOGI/LDAP/BASEDN"),0);
}

const char * blogi::Config::getlpfilter(){
    return _PlsConfig->getValue(_PlsConfig->getKey("/BLOGI/LDAP/LOGINFILTER"),0);
}

const char * blogi::Config::gethttpaddr(){
    return _PlsConfig->getValue(_PlsConfig->getKey("/BLOGI/HTTP/BIND"),0);
}

int blogi::Config::gethttpport(){
    return _PlsConfig->getIntValue(_PlsConfig->getKey("/BLOGI/HTTP/PORT"),0);
}

int blogi::Config::gethttpmaxcon(){
    return _PlsConfig->getIntValue(_PlsConfig->getKey("/BLOGI/HTTP/MAXCON"),0);
}

