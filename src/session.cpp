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

#include <cstring>

#include <httppp/exception.h>

#include "session.h"

blogi::Session::Session(){
    _firstSessionData=nullptr;
    _lastSessionData=nullptr;
}

blogi::Session::~Session(){
    delete _firstSessionData;
}

const char *blogi::Session::createSession(const char *uid){
    if(_firstSessionData){
        _lastSessionData->_nextSessionData= new SessionData();
        _lastSessionData=_lastSessionData->_nextSessionData;
    }else{
        _firstSessionData=new SessionData();
        _lastSessionData=_firstSessionData;
    }
    uuid_unparse(_lastSessionData->_sessionid,_buffer);
    addSessionData(_buffer,"uid",uid,strlen(uid)+1);
    return  _buffer;
};

void blogi::Session::addSessionData(const char *sessionid,const char *key,const char *value,size_t size){
    for(SessionData *curses=_firstSessionData; curses; curses=curses->_nextSessionData){
        uuid_t sin;
        uuid_parse(sessionid,sin);
        if(uuid_compare(curses->_sessionid,sin)==0){
            if(curses->_firstData){
                curses->_lastData->_nextData=new SessionData::Data;
                curses->_lastData=curses->_lastData->_nextData;
            }else{
                curses->_firstData=new SessionData::Data;
                curses->_lastData=curses->_firstData;
            }
            curses->_lastData->_Data.resize(size);
            curses->_lastData->_Data.insert(0,value,size);
            snprintf(curses->_lastData->_key,255,"%s",key);
            return;
        }

    }

    libhttppp::HTTPException excep;
    excep[libhttppp::HTTPException::Error] << "addSessionData Session:" <<  sessionid << "not found!";
    throw excep;
};

void blogi::Session::getSessionData(const char *sessionid,const char *key,std::string &value){
    for(SessionData *curses=_firstSessionData; curses; curses=curses->_nextSessionData){
        uuid_t sin;
        uuid_parse(sessionid,sin);
        if(uuid_compare(curses->_sessionid,sin)==0){
            for(SessionData::Data *curdat=curses->_firstData; curdat; curdat=curdat->_nextData){
                if(strcmp(key,curdat->_key)==0){
                    value=curdat->_Data;
                    return;
                }
            }
        }
    }
    libhttppp::HTTPException excep;
    excep[libhttppp::HTTPException::Error] << "getSessionData Session:" <<  sessionid << "not found!";
    throw excep;
};

blogi::Session::SessionData::SessionData(){
    _firstData=nullptr;
    _lastData=nullptr;
    _nextSessionData=nullptr;
    uuid_generate(_sessionid);
};

blogi::Session::SessionData::~SessionData(){
    delete _nextSessionData;
};

blogi::Session::SessionData::Data::Data(){
    _nextData=nullptr;
}

blogi::Session::SessionData::Data::~Data(){
    delete   _nextData;
}

