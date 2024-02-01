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

#include <algorithm>

#include "editor.h"

blogi::Editor::Editor(Config *conf)
{
    _firstIcon=nullptr;
    _lastIcon=nullptr;
    _Config=conf;
}

blogi::Editor::~Editor(){
    delete _firstIcon;
    delete _lastIcon;
}

void blogi::Editor::addIcon(const unsigned char* icon, size_t iconsize,const char *name,const char *type, const char* description){
    if(_firstIcon){
        _lastIcon->_nextIcon=new Icons;
        _lastIcon=_lastIcon->_nextIcon;
    }else{
        _firstIcon= new Icons;
        _lastIcon=_firstIcon;
    }
    _lastIcon->_Icon.resize(iconsize);
    std::copy(icon,icon+iconsize,std::inserter<std::string>(_lastIcon->_Icon,_lastIcon->_Icon.begin()));
    _lastIcon->_Name=name;
    _lastIcon->_Type=type;
    _lastIcon->_Description=description;
}

void blogi::Editor::displayEditor(const char* inputname,const char *value, libhtmlpp::HtmlString& target){
    char url[512];
    target << "<div id=\"editor\"><ul> ";
    for(Icons *curicon=_firstIcon; curicon; curicon=curicon->_nextIcon){
        target << "<li><img src=\"" << _Config->buildurl("editor/icon/",url,512) << curicon->_Name
               << "." << curicon->_Type << "\" alt=\"" << curicon->_Description << "\" /></li>";
    }
    target << "</ul><textarea name=\"" << inputname << "\">";
    if(value)
        target << value;
    target <<"</textarea></div>";
}

void blogi::Editor::Controller(netplus::con* curcon, libhttppp::HttpRequest* req){
    char url[512];
    if(strncmp(req->getRequestURL(),_Config->buildurl("editor/icon/",url,512),
       strlen(_Config->buildurl("editor/icon/",url,512) ))==0){
        std::string reqfile;
        std::copy(req->getRequestURL()+strlen(_Config->buildurl("editor/icon/",url,512)),
                  req->getRequestURL()+strlen(req->getRequestURL()),std::inserter<std::string>(reqfile,reqfile.begin()));
        for(Icons *curicon=_firstIcon; curicon; curicon=curicon->_nextIcon){
            if(reqfile.substr(0,reqfile.find("."))==curicon->_Name){
                libhttppp::HttpResponse resp;
                resp.setVersion(HTTPVERSION(1.1));
                resp.setState(HTTP200);
                resp.setContentType(std::string("image/").append(curicon->_Type).c_str());
                resp.send(curcon,curicon->_Icon.c_str(),curicon->_Icon.length());
                return;
            }
        }
    }
}
