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
#include <cstring>
#include <vector>

#pragma once

namespace blogi {
    class Database;

    class SQL {
    public:
        SQL();
        ~SQL();
        SQL& operator<<(const char *sql);
        SQL& operator<<(int sql);
        SQL& operator=(const char *sql);

        SQL& escaped(const char *text);

        const char *c_str();
        void clear();
        bool empty();
        size_t size();
        void append(const char* data, size_t datalen);

    private:
        std::vector<char> _SQL;
        std::vector<char> _CStr;
        friend class      Database;
    };

    class DBResult2;

    class DBResult {
    public:
        DBResult();

        ~DBResult(){
            clear();
        }

        void clear(){
            Data *curres=firstRow;
            while(curres){
                Data *next=curres->nextData;
                curres->nextData=nullptr;
                delete curres;
                curres = next;
            }
            firstRow=nullptr;
        };

        DBResult2 operator[](int value);

        struct Data{
            Data(int crow,int ccol,const char *value, int len){
                row=crow;
                col=ccol;
                std::copy(value,value+len,
                          std::inserter<std::vector<char>>(Column,Column.begin()));
                Column.push_back('\0');
                nextData=nullptr;
            }

            int               row;
            int               col;
            std::vector<char> Column;
            Data             *nextData;
            friend class DBResult;
        };
        Data          *firstRow;
        friend Database;
        friend DBResult2;
    };

    class DBResult2{
    public:
        DBResult2(DBResult *res,int value){
            result=res;
            row=value;
        }

        ~DBResult2(){
        }

        const char *operator[](int value2){
            for(DBResult::Data *pos=result->firstRow; pos; pos=pos->nextData){
                if( row==pos->row &&  pos->col==value2){
                    return pos->Column.data();
                }
            }
            return nullptr;
        }
    private:
        DBResult *result;
        int       row;
    };

    class Database {
    public:
        Database(const char *connstr);
        virtual ~Database();
        virtual int exec(SQL *sql,DBResult &res)=0;
        virtual const char *getDriverName()=0;
        virtual const char *autoincrement()=0;
        virtual bool isConnected()=0;
        virtual void reset() =0;
    };
};
