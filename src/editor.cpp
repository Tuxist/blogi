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

namespace blogi {
    class EditorIcons {
    public:
        EditorIcons(){

        };
        ~EditorIcons(){

        }
    private:
        class Icons {
            Icons(){
                _nextIcon=nullptr;
            }

            ~Icons(){
                delete _nextIcon;
            }

            std::string  _Icon;
            std::string  _Type;
            std::string  _Url;
            std::string  _Description;
            Icons       *_nextIcon;
            friend class EditorIcons;
        };

        void addIcon(const unsigned char* icon, size_t iconsize,const char *type,const char *url, const char* description){
            if(_firstIcon){
                _lastIcon->_nextIcon=new Icons;
                _lastIcon=_lastIcon->_nextIcon;
            }else{
                _firstIcon= new Icons;
                _lastIcon=_firstIcon;
            }
            _lastIcon->_Icon.resize(iconsize);
            std::copy(icon,icon+iconsize,_lastIcon->_Icon.begin());
            _lastIcon->_Type=type;
            _lastIcon->_Description=description;
        }

        Icons *_firstIcon;
        Icons *_lastIcon;

        friend class Editor;
    } _EditorIcons;
};

blogi::Editor::Editor()
{
}

blogi::Editor::~Editor(){
}

void blogi::Editor::addIcon(const unsigned char* icon, size_t iconsize,const char *type,const char *url, const char* description){
    _EditorIcons.addIcon(icon,iconsize,type,url,description);
}

void blogi::Editor::displayEditor(const char* inputname, libhtmlpp::HtmlElement* target){
}
