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

blogi::Auth::Auth(blogi::Database *pcon,blogi::Session *session){
    _dbconn=pcon;
    _session=session;
}

blogi::Auth::~Auth(){

}

bool blogi::Auth::login(const char* username, const char* password, std::string& ssid){
#ifdef LDAPSUPPORT
    if(!ldapLogin(username,password,ssid))
        return locallogin(username,password,ssid);
    else
        return true;
#else
    return locallogin(username,password,ssid);
#endif
}

bool blogi::Auth::locallogin(const char* username, const char* password, std::string& ssid){
    blogi::SQL sql;
    blogi::DBResult res;
    sql = "SELECT sid,id,username FROM users WHERE username='"; sql.escaped(username) << "' LIMIT 1;";
    return false;
}

#ifdef LDAPSUPPORT
bool blogi::Auth::ldapLogin(const char *username,const char *password,std::string &ssid){
    libhttppp::HTTPException excep;
    timeval ltimeout;
    ltimeout.tv_sec = 5;

    LDAP* userldap;
    LDAPControl* userserverctls = nullptr;
    LDAPControl* userclientctls = nullptr;

    int ulerr = ldap_initialize(&userldap, Config::getInstance()->getlphost());

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
    ulerr = ldap_sasl_bind_s(userldap, username, LDAP_SASL_SIMPLE,
                             &cred, &userserverctls, &userclientctls, &servercred);

    if (ulerr != LDAP_SUCCESS) {
        excep[libhttppp::HTTPException::Note] << "User Ldap Login failed: "
        << ldap_err2string(ulerr);
        throw excep;
    }

    char* attrs[] = { (char*)"objectSid",(char*)"userPrincipalName",(char*)"mail",NULL };
    LDAPMessage* answer, * entry;
    struct timeval timeout;

    timeout.tv_sec = 1;

    ulerr = ldap_search_ext_s(userldap, Config::getInstance()->getlpbasedn(), LDAP_SCOPE_SUBTREE, Config::getInstance()->getlpfilter(), attrs, 0, &userserverctls, &userclientctls, &timeout, 0, &answer);

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
    char sid[512],email[255];
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
                 }
                 if (strcmp(attribute, "mail") == 0) {
                     memcpy(&email,values[0]->bv_val,values[0]->bv_len);
                     email[values[0]->bv_len]='\0';
                 }
             }
        }
        ldap_value_free_len(values);

        printSID(mysid,sid,512);
        destroySID(mysid);
        std::cout << "User : " << sid << " are logged in" << std::endl;
        ssid=sid;
        blogi::SQL sql;
        blogi::DBResult res;
        sql = "SELECT sid,id FROM users WHERE sid='"; sql << sid << "' LIMIT 1;";

        if (_dbconn->exec(&sql,res) < 1) {
           sql <<  "INSERT INTO users (sid,username,email,authprovider) VALUES ('" << sid <<"','" << username << "','" << email <<"','1');";
           _dbconn->exec(&sql,res);
        };

        _session->createSession(sid);

        ldap_memfree(dn);
        ldap_msgfree(answer);
        ldap_unbind_ext_s(userldap, &userserverctls, &userclientctls);
        return true;
}
#endif

bool blogi::Auth::isLoggedIn(libhttppp::HttpRequest *curreq,std::string &sessionid){
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
