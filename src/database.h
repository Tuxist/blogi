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
#include <string>

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
    private:
        std::string _SQL;
        friend class Database;
    };

    class DBResult2;

    class DBResult {
    public:
        DBResult();
        ~DBResult();

        DBResult2 operator[](int value);

        struct Data{
            Data(int crow,int ccol,const char *value, int len){
                row=crow;
                col=ccol;
                Column=new char[len+1];
                memcpy(Column,value,len);
                Column[len]='\0';
                nextData=nullptr;
            }
            ~Data(){
                delete Column;
                delete nextData;
            }

            int         row;
            int         col;
            char       *Column;
            Data       *nextData;
            friend class DBResult;
        };
        int            columns;
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

        const char *operator[](int value2){
            for(DBResult::Data *pos=result->firstRow; pos; pos=pos->nextData){
                if( row==pos->row &&  pos->col==value2){
                    return pos->Column;
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
    };
};
