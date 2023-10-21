/*******************************************************************************
Copyright (c) 2023, Jan Koester jan.koester@gmx.net
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

#include <string>

#include <uuid/uuid.h>

#pragma once

namespace blogi {
  class Session {
  public:
    Session();
    ~Session();

    const char *createSession(const char *uid);
    void addSessionData(const char *sessionid,const char *key,const char *value,size_t size);
    void getSessionData(const char *sessionid,const char *key,std::string &value);

  private:
    struct SessionData{
      SessionData();
      ~SessionData();
      struct Data{
        Data();
        ~Data();
        char         _key[255];
        std::string  _Data;
        Data        *_nextData;
      };
      uuid_t       _sessionid;
      Data        *_firstData;
      Data        *_lastData;
      SessionData *_nextSessionData;
    };

    SessionData *_firstSessionData;
    SessionData *_lastSessionData;
    char         _buffer[255];
  };
};
