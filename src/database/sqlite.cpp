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

#include <atomic>

#include <httppp/exception.h>

#include <sqlite3.h>

#include "../database.h"

namespace blogi {

    std::atomic<bool> sqllock(false);

    class SQLite : public Database{
    public:
        SQLite(const char *constr) : Database (constr) {
            while( sqllock.exchange(true, std::memory_order_acquire) );
            int status=sqlite3_open(constr,&_dbconn);
            if (status != SQLITE_OK ){
                libhttppp::HTTPException exp;
                exp[libhttppp::HTTPException::Critical] << sqlite3_errmsg(_dbconn);
                sqlite3_close(_dbconn);
                throw exp;
            }
            sqllock.store(false);

        }

        ~SQLite(){
            sqlite3_close(_dbconn);
        }

        int exec(SQL *sql,DBResult &res){
            while( sqllock.exchange(true, std::memory_order_acquire) );
            char *ssql;
            ssql=new char[sql->length()];
            memcpy(ssql,sql->c_str(),sql->length());
            sqlite3_stmt *prep;
            int rcount = 0;

            if(res.firstRow){
                delete res.firstRow;
            }

            res.firstRow=nullptr;
            DBResult::Data *lastdat;

            const char *cssql=ssql;

            do{
                const char *sqlptr=nullptr;
                int pstate=sqlite3_prepare_v3(_dbconn,cssql,sql->length(),-1,&prep,&sqlptr);

                cssql=sqlptr;

                if(pstate == SQLITE_ERROR) {
                    libhttppp::HTTPException exp;
                    exp[libhttppp::HTTPException::Critical] << sqlite3_errmsg(_dbconn);
                    sqlite3_finalize(prep);
                    delete[] ssql;
                    sqllock.store(false);
                    throw exp;
                }

                if(!prep)
                    continue;

                do {
                    int pcode = sqlite3_step(prep);

                    if(pcode==SQLITE_ERROR){
                        libhttppp::HTTPException exp;
                        exp[libhttppp::HTTPException::Critical] << sqlite3_errmsg(_dbconn);
                        delete[] ssql;
                        sqllock.store(false);
                        throw exp;
                    }

                    if(pcode==SQLITE_BUSY){
                        continue;
                    }

                    res.columns=sqlite3_data_count(prep);

                    int i;

                    for(i=0; i < res.columns; ++i){
                        if(!res.firstRow){
                            res.firstRow = new DBResult::Data(rcount,i,(const char*)sqlite3_column_text(prep,i),sqlite3_column_bytes(prep,i));
                            lastdat=res.firstRow;
                        }else{
                            lastdat->nextData=new DBResult::Data(rcount,i,(const char*)sqlite3_column_text(prep,i),sqlite3_column_bytes(prep,i));
                            lastdat=lastdat->nextData;
                        }
                    }

                    if(res.columns>0)
                        ++rcount;

                    sqlite3_stmt *next;
                    next=sqlite3_next_stmt(_dbconn, prep);
                    sqlite3_finalize(prep);
                    prep=next;
                }while(prep);
            }while(cssql<ssql+sql->length());
            sqllock.store(false);
            delete[] ssql;
            return rcount;
        };

        const char *getDriverName(){
            return "sqlite";
        };

        const char *autoincrement(){
            return "AUTOINCREMENT";
        }

    private:

        sqlite3  *_dbconn;
    };
}

