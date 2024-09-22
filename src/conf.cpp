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

blogi::Config::Config(const char *path) : confplus::Config(path){
    if(getKey("/BLOGI/PLUGINDIR")){
        for(int i =0; i<getElements(getKey("/BLOGI/PLUGINDIR")); ++i){
             _PlgDir.push_back(getValue(getKey("/BLOGI/PLUGINDIR"),i));
        }
    }

    if(getKey("/BLOGI/DATABASE/DRIVER"))
        _DBDriver=getValue(getKey("/BLOGI/DATABASE/DRIVER"),0);

    if(getKey("/BLOGI/DATABASE/CONNECTION"))
        _DBConnection=getValue(getKey("/BLOGI/DATABASE/CONNECTION"),0);

    if(getKey("/BLOGI/LDAP/DOMAIN"))
        _LDAPDomain=getValue(getKey("/BLOGI/LDAP/DOMAIN"),0);

    if(getKey("/BLOGI/LDAP/HOST"))
        _LDAPHost=getValue(getKey("/BLOGI/LDAP/HOST"),0);

    if(getKey("/BLOGI/LDAP/BASEDN"))
        _LDAPBase=getValue(getKey("/BLOGI/LDAP/BASEDN"),0);

    if(getKey("/BLOGI/LDAP/LOGINFILTER"))
        _LDAPFilter=getValue(getKey("/BLOGI/LDAP/LOGINFILTER"),0);

    if(getKey("/BLOGI/HTTP/BIND"))
        _HttpBind=getValue(getKey("/BLOGI/HTTP/BIND"),0);

    if(getKey("/BLOGI/HTTP/PORT"))
        _HttpPort=getIntValue(getKey("/BLOGI/HTTP/PORT"),0);
    else
        _HttpPort=-1;


    if(getKey("/BLOGI/HTTP/MAXCON"))
        _MaxCon=getIntValue(getKey("/BLOGI/HTTP/MAXCON"),0);
    else
        _MaxCon=-1;

    try{
        _RedisHost=getValue(getKey("/BLOGI/REDIS/HOST"),0);
        _RedisPassword=getValue(getKey("/BLOGI/REDIS/PASSWORD"),0);
    }catch(...){
    }

    if(getKey("/BLOGI/REDIS/PORT"))
        _RedisPort=getIntValue(getKey("/BLOGI/REDIS/PORT"),0);
    else
        _RedisPort=-1;

    if(getKey("/BLOGI/REDIS/TIMEOUT"))
        _RedisTimeout=getIntValue(getKey("/BLOGI/REDIS/TIMEOUT"),0);
    else
        _RedisTimeout=0;

    if(getKey("/BLOGI/HTTP/URL"))
        _HttpUrl=getValue(getKey("/BLOGI/HTTP/URL"),0);

    if(getKey("/BLOGI/HTTP/PREFIX"))
        _HttpPrefix=getValue(getKey("/BLOGI/HTTP/PREFIX"),0);

    if(getKey("/BLOGI/TEMPLATE"))
        _Template=getValue(getKey("/BLOGI/TEMPLATE"),0);

    if(getKey("/BLOGI/STARTPAGE"))
        _StartPage=getValue(getKey("/BLOGI/STARTPAGE"),0);

    try{
        if(getKey("/BLOGI/HTTP/SSLCERTPATH"))
            _SSLCertpath=getValue(getKey("/BLOGI/HTTP/SSLCERTPATH"),0);

        if(getKey("/BLOGI/HTTP/SSLKEYPATH"))
            _SSLKeypath=getValue(getKey("/BLOGI/HTTP/SSLKEYPATH"),0);
    }catch(...){
    }

    if(getKey("/BLOGI/DOMAIN/NAME"))
        _Domain=getValue(getKey("/BLOGI/DOMAIN/NAME"),0);
}

blogi::Config::~Config(){

}


const char * blogi::Config::buildurl(const char *url,char *buffer,size_t size){
    if(!getprefix())
        return nullptr;
    snprintf(buffer,size,"%s/%s",getprefix(),url);
    return buffer;
}

const char *blogi::Config::getplgdir(size_t el){
    return _PlgDir[el].c_str();
}

size_t blogi::Config::getplgdirs(){
    return _PlgDir.size();
}

const char * blogi::Config::getdbdriver(){
    if(_DBDriver.empty())
        return nullptr;
    return _DBDriver.c_str();
}

const char * blogi::Config::getdbopts(){
    if(_DBConnection.empty())
        return nullptr;
    return _DBConnection.c_str();
}

const char * blogi::Config::getlpdomain(){
    if(_LDAPDomain.empty())
        return nullptr;
    return _LDAPDomain.c_str();
}

const char *blogi::Config::getlphost(){
    if(_LDAPHost.empty())
        return nullptr;
    return _LDAPHost.c_str();
}

const char * blogi::Config::getlpbasedn(){
    if(_LDAPBase.empty())
        return nullptr;
    return _LDAPBase.c_str();
}

const char * blogi::Config::getlpfilter(){
    if(_LDAPFilter.empty())
        return nullptr;
    return _LDAPFilter.c_str();
}

const char * blogi::Config::gethttpaddr(){
    if(_HttpBind.empty())
        return nullptr;
    return _HttpBind.c_str();
}

int blogi::Config::gethttpport(){
    return _HttpPort;
}

int blogi::Config::gethttpmaxcon(){
    return _MaxCon;
}

const char * blogi::Config::getRedisHost(){
    if(_RedisHost.empty())
        return nullptr;
    return _RedisHost.c_str();
}

const char * blogi::Config::getRedisPassword(){
    if(_RedisPassword.empty())
        return nullptr;
    return _RedisPassword.c_str();
}

int blogi::Config::getRedisPort(){
    return _RedisPort;
}

int blogi::Config::getRedisTimeout(){
    return _RedisTimeout;
}

const char * blogi::Config::getsiteurl(){
    if(_HttpUrl.empty())
        return nullptr;
    return _HttpUrl.c_str();
}

const char * blogi::Config::getprefix(){
    if(_HttpPrefix.empty())
        return nullptr;
    return _HttpPrefix.c_str();
}


const char * blogi::Config::gettemplate(){
    if(_Template.empty())
        return nullptr;
    return _Template.c_str();
}

const char * blogi::Config::getstartpage(){
    if(_StartPage.empty())
        return nullptr;
    return _StartPage.c_str();
}

const char * blogi::Config::getsslcertpath(){
    if(_SSLCertpath.empty())
        return nullptr;
    return _SSLCertpath.c_str();
}

const char * blogi::Config::getsslkeypath(){
    if(_SSLKeypath.empty())
        return nullptr;
    return _SSLKeypath.c_str();
}

const char * blogi::Config::getDomain(){
    if(_Domain.empty())
        return nullptr;
    return _Domain.c_str();
}
