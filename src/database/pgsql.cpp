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

#include <mutex>

#include <httppp/exception.h>

#include <libpq-fe.h>

#include "../database.h"

namespace blogi {

    std::mutex g_lock_mutex;

    class Postgresql : public Database{
    public:
        Postgresql(const char *constr) : Database (constr) {
            _dbconn = PQconnectdb(constr);
            if (PQstatus(_dbconn) != CONNECTION_OK){
                libhttppp::HTTPException exp;
                exp[libhttppp::HTTPException::Critical] << PQerrorMessage(_dbconn);
                PQfinish(_dbconn);
                throw exp;
            }
        }

        ~Postgresql(){
            PQfinish(_dbconn);
        }

        int exec(SQL *sql,DBResult &res) override{
            PGresult *pres = PQexec(_dbconn,sql->c_str());
            int pstate=PQresultStatus(pres);

            if(pstate==PGRES_FATAL_ERROR || pstate==PGRES_BAD_RESPONSE) {
                 libhttppp::HTTPException exp;
                 exp[libhttppp::HTTPException::Critical] << PQerrorMessage(_dbconn);
                 PQclear(pres);
                 throw exp;
            }

            res.clear();

            DBResult::Data *lastdat=nullptr;

            int rcount=PQntuples(pres);

            for(int i = 0; i < rcount; ++i ){
                for(int ii=0; ii < PQnfields(pres); ++ii){
                    if(!res.firstRow){
                       res.firstRow = new DBResult::Data(i,ii,PQgetvalue(pres,i,ii),PQgetlength(pres,i,ii));
                       lastdat=res.firstRow;
                    }else{
                       lastdat->nextData=new DBResult::Data(i,ii,PQgetvalue(pres,i,ii),PQgetlength(pres,i,ii));
                       lastdat=lastdat->nextData;
                    }
                }
            }

            PQclear(pres);

            return rcount;
        };

        const char *getDriverName() override{
            return "pgsql";
        };

        const char *autoincrement() override{
            return "GENERATED BY DEFAULT AS IDENTITY";
        }

        bool isConnected() override{
            const std::lock_guard<std::mutex> lock(g_lock_mutex);
            if(PQstatus(_dbconn)==CONNECTION_OK)
                return true;
            return false;
        }

        void reset() override{
            const std::lock_guard<std::mutex> lock(g_lock_mutex);
            PQreset(_dbconn);
        }

    private:
        PGconn      *_dbconn;
    };
}
