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

#include <httppp/exception.h>

#include <sqlite3.h>

#include "../database.h"

namespace blogi {
    class SQLite : public Database{
    public:
        SQLite(const char *constr) : Database (constr) {
            int status=sqlite3_open(constr,&_dbconn);
            if (status){
                libhttppp::HTTPException exp;
                exp[libhttppp::HTTPException::Critical] << sqlite3_errmsg(_dbconn);
                sqlite3_close(_dbconn);
                throw exp;
            }
        }

        ~SQLite(){
            sqlite3_close(_dbconn);
        }

        int exec(SQL *sql,DBResult &res){
            const char *zErrMsg = 0;

            sqlite3_stmt *prep;

            int pstate=sqlite3_prepare(_dbconn,sql->c_str(),sql->length(),&prep,&zErrMsg);


            if(pstate) {
                 libhttppp::HTTPException exp;
                 exp[libhttppp::HTTPException::Critical] << zErrMsg;
                 throw exp;
            }

            if(sqlite3_step(prep)){
                 libhttppp::HTTPException exp;
                 exp[libhttppp::HTTPException::Critical] << sqlite3_errmsg(_dbconn);
                 throw exp;
            }

            if(res.firstRow){
                delete res.firstRow;
            }

            res.firstRow=nullptr;
            DBResult::Data *lastdat;

            res.columns=sqlite3_data_count(prep);

            int rcount=sqlite3_column_count(prep);

            for(int i = 0; i < rcount; ++i ){
                for(int ii=0; ii < res.columns; ++ii){
                    if(!res.firstRow){
                       res.firstRow = new DBResult::Data(i,ii,(const char*)sqlite3_column_text(prep,ii),sqlite3_column_bytes(prep,ii));
                       lastdat=res.firstRow;
                    }else{
                       lastdat->nextData=new DBResult::Data(i,ii,(const char*)sqlite3_column_text(prep,ii),sqlite3_column_bytes(prep,ii));
                       lastdat=lastdat->nextData;
                    }
                }
                prep=sqlite3_next_stmt(_dbconn,prep);
            }

            sqlite3_finalize(prep);
            return rcount;
        };

        const char *getDriverName(){
            return "sqlite";
        };

    private:

        sqlite3  *_dbconn;
    };
}

