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

#include <iostream>
#include <cstring>

#include <sys/time.h>

#include <secureid.h>

#include <httppp/exception.h>

#include "auth.h"
#include "conf.h"
#include "theme.h"

blogi::Auth::Auth(blogi::Database **pcon,blogi::Session *session,blogi::Config *cfg){
    _dbconn=pcon;
    _session=session;
    _config=cfg;
    blogi::SQL sql;
    blogi::DBResult res;
    sql << "CREATE TABLE IF NOT EXISTS users ("
        <<      "id integer PRIMARY KEY " << pcon[0]->autoincrement() << ","
        <<      "sid character varying(255),"
        <<      "username character varying(255),"
        <<      "email character varying(255),"
        <<      "displayname character varying(255) NOT NULL,"
        <<      "authprovider integer DEFAULT 0 NOT NULL,"
        <<      "password character varying(255)"
        << ");";
    _dbconn[0]->exec(&sql,res);
}

blogi::Auth::~Auth(){

}

bool blogi::Auth::login(const int tid,const char* username, const char* password, std::string& ssid){
#ifndef LDAPSUPPORT
    return locallogin(username,password,ssid);
#else
    bool ret=ldapLogin(tid,username,password,ssid);
    if(!ret)
        return locallogin(tid,username,password,ssid);
    return ret;
#endif
}

bool blogi::Auth::locallogin(const int tid,const char* username, const char* password, std::string& ssid){
    blogi::SQL sql;
    blogi::DBResult res;
    sql = "SELECT sid,id,username FROM users WHERE username='"; sql.escaped(username) << "' LIMIT 1;";
    return false;
}

#ifdef LDAPSUPPORT
bool blogi::Auth::ldapLogin(const int tid,const char *username,const char *password,std::string &ssid){
    libhttppp::HTTPException excep;
    timeval ltimeout;
    ltimeout.tv_sec = 5;

    LDAP* userldap;
    LDAPControl* userserverctls = nullptr;
    LDAPControl* userclientctls = nullptr;

    int ulerr = ldap_initialize(&userldap, _config->getlphost());

    if (ulerr != LDAP_SUCCESS) {
        excep[libhttppp::HTTPException::Critical] << "User Ldap init failed: "
        << ldap_err2string(ulerr);
        throw excep;
    }

    int ldap_vers = LDAP_VERSION3;
    ldap_set_option(userldap, LDAP_OPT_PROTOCOL_VERSION, &ldap_vers);

    ulerr = ldap_start_tls_s(userldap, &userserverctls, &userclientctls);

    if (ulerr != LDAP_SUCCESS) {
         excep[libhttppp::HTTPException::Critical] << "User Ldap Tls failed: "
         << ldap_err2string(ulerr);
         throw excep;
    }

    berval cred, * servercred;
    cred.bv_val = (char*)password;
    cred.bv_len = strlen(password);
    ulerr = ldap_sasl_bind_s(userldap, username, nullptr,
                             &cred, &userserverctls, &userclientctls, &servercred);

    if (ulerr != LDAP_SUCCESS) {
        excep[libhttppp::HTTPException::Note] << "User Ldap Login failed: "
        << ldap_err2string(ulerr);
        throw excep;
    }

    char* attrs[] = { (char*)"objectSid",(char*)"userPrincipalName",(char*)"mail",(char*)"gecos",nullptr };
    LDAPMessage* answer, * entry;
    struct timeval timeout;

    timeout.tv_sec = 1;
    timeout.tv_usec =0;

    ulerr = ldap_search_ext_s(userldap, _config->getlpbasedn(), LDAP_SCOPE_SUBTREE, _config->getlpfilter(), attrs, 0, &userserverctls, &userclientctls, &timeout, 0, &answer);

    if (ulerr != LDAP_SUCCESS) {
        excep[libhttppp::HTTPException::Note] << "<span>User can'T find uid !</span>";
        throw excep;
    }

    size_t entries_found = ldap_count_entries(userldap, answer);

    if (entries_found <= 0) {
        excep[libhttppp::HTTPException::Note] << "<span>User can'T find uid !</span>";
        throw excep;
    }

    const char* sessid = nullptr;
    char sid[512],email[255],displayname[512];
    BerElement* ber;
    char* dn;
    char* attribute;
    bool auth = false;

    for (entry = ldap_first_entry(userldap, answer);
         entry != NULL; entry = ldap_next_entry(userldap, entry)) {

        dn = ldap_get_dn(userldap, entry);
        berval** values;


        for (attribute = ldap_first_attribute(userldap, entry, &ber);
         attribute != nullptr; attribute = ldap_next_attribute(userldap, entry, ber)) {
            if ((values = ldap_get_values_len(userldap, entry, attribute))) {
                if (strcmp(attribute, "userPrincipalName") == 0) {
                    for (int i = 0; values[i]; i++) {
                        if (strncmp(username, values[i]->bv_val, values[i]->bv_len) == 0) {
                            ldap_value_free_len(values);
                            goto LDAPLOGINUSERFOUND;
                        }
                    }
                }
            }
            ldap_value_free_len(values);
        }
    }

    ldap_memfree(dn);
    ldap_msgfree(answer);
    ldap_unbind_ext_s(userldap, &userserverctls, &userclientctls);

    return false;


LDAPLOGINUSERFOUND:
         SID* mysid;
         initSID(&mysid);
         berval** values;
         for (attribute = ldap_first_attribute(userldap, entry, &ber);
              attribute != nullptr; attribute = ldap_next_attribute(userldap, entry, ber)) {
             if ((values = ldap_get_values_len(userldap, entry, attribute))) {
                 if (strcmp(attribute, "objectSid") == 0) {
                     SIDcpy(mysid,(SID*)values[0]->bv_val);
                 }else if (strcmp(attribute, "mail") == 0) {
                     memcpy(&email,values[0]->bv_val,values[0]->bv_len);
                     email[values[0]->bv_len]='\0';
                 }else if (strcmp(attribute, "gecos") == 0) {
                     memcpy(&displayname,values[0]->bv_val,values[0]->bv_len);
                     displayname[values[0]->bv_len]='\0';
                 }
             }
        }
        ldap_value_free_len(values);

        printSID(mysid,sid,512);
        destroySID(mysid);
        ssid=sid;
        blogi::SQL sql;
        blogi::DBResult res;
        sql = "SELECT sid,id FROM users WHERE sid='"; sql << sid << "' LIMIT 1;";

        if (_dbconn[tid]->exec(&sql,res) < 1) {
           sql <<  "INSERT INTO users (sid,username,email,authprovider,displayname) VALUES ('" << sid <<"','" << username << "','" << email <<"','1','" << displayname << "');";
           _dbconn[tid]->exec(&sql,res);
        };

        _session->createSession(sid);

        std::cout << "User : " << sid << " are logged in" << std::endl;

        ldap_memfree(dn);
        ldap_msgfree(answer);
        ldap_unbind_ext_s(userldap, &userserverctls, &userclientctls);
        return true;
}
#endif

bool blogi::Auth::isLoggedIn(const int tid,libhttppp::HttpRequest *curreq,std::string &sessionid){
        libhttppp::HttpCookie cookie;
        cookie.parse(curreq);
        const char *tmp=nullptr;
        for(libhttppp::HttpCookie::CookieData *curcookie=cookie.getfirstCookieData();
            curcookie; curcookie=curcookie->nextCookieData()){
            if(curcookie->getKey() && strcmp(curcookie->getKey(),"sessionid")==0)
               tmp=curcookie->getValue();
        }
        if(!tmp)
            return false;
        try{
            std::string storeid;
            _session->getSessionData(tmp,"uid",storeid);
            if(!storeid.empty()){
                sessionid=tmp;
                return true;
            }
        }catch(...){};
        return false;
}
