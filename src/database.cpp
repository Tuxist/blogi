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

#include "database.h"

blogi::Database::Database(const char* connstr){
}

blogi::Database::~Database(){
}


blogi::SQL::SQL(){
}

blogi::SQL::~SQL()
{
}

blogi::SQL & blogi::SQL::operator<<(const char* sql){
    _SQL.append(sql);
    return *this;
}

blogi::SQL & blogi::SQL::operator<<(int sql){
    char buf[512];
    snprintf(buf,512,"%d",sql);
    _SQL.append(buf);
    return *this;
}

blogi::SQL & blogi::SQL::operator=(const char* sql){
    _SQL=sql;
    return *this;
}

blogi::SQL & blogi::SQL::escaped(const char* text){
    size_t tlen=strlen(text);
    for(size_t i = 0; i < tlen; ++i){
        if(text[i]=='\''){
            _SQL.append("''");
        }else{
            _SQL.push_back(text[i]);
        }
    }
    return *this;
}


const char * blogi::SQL::c_str(){
    return _SQL.c_str();
}

void blogi::SQL::clear(){
    _SQL.clear();
}

bool blogi::SQL::empty(){
    return _SQL.empty();
}


blogi::DBResult::DBResult(){
    firstRow=nullptr;
}

blogi::DBResult::~DBResult(){
}

blogi::DBResult2 blogi::DBResult::operator[](int value){
    DBResult2 result(this,value);
    return result;
}

int blogi::Database::exec(SQL *sql,DBResult &res){
    return -1;
}
