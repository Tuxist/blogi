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

#include <dirent.h>
#include <dlfcn.h>

#include "plugin.h"

//musl hack
blogi::PluginApi::~PluginApi(){
}

bool blogi::PluginApi::haveSettings(){
    return false;
}

void blogi::PluginApi::Settings(libhttppp::HttpRequest *req,libhtmlpp::HtmlString &setdiv){
   return;
};

bool blogi::PluginApi::Controller(netplus::con *curcon,libhttppp::HttpRequest *req){
    return false;
}

void blogi::PluginApi::setArgs(blogi::PluginArgs* args){
    Args=args;
}

blogi::Plugin::Plugin(){
    _firstPlugin=nullptr;
}


void blogi::Plugin::loadPlugins(const char *path,PluginArgs *args){
    DIR *directory = opendir(path);
    struct dirent *direntStruct;

    if (directory != nullptr) {
        while((direntStruct = readdir(directory))) {
            if((direntStruct->d_name[0]=='.' && strlen(direntStruct->d_name)==1) ||
              ((direntStruct->d_name[0]=='.') &&  (direntStruct->d_name[1]=='.') &&
              strlen(direntStruct->d_name)==2))
                continue;
            try{
                std::string ppath = path;
                ppath.append("/");
                ppath.append(direntStruct->d_name);

                PluginData *ldplg=new PluginData();

                ldplg->pldata = dlopen(ppath.c_str(),RTLD_LAZY);
                if (!ldplg->pldata) {
                    delete ldplg;
                    libhttppp::HTTPException err;
                    err[libhttppp::HTTPException::Error] << "Cannot load library";
                    throw err;
                }
                // load the symbols
                create_t* create_plugin= (create_t*) dlsym(ldplg->pldata, "create");
                const char* dlsym_error = dlerror();
                if (dlsym_error) {
                    delete ldplg;
                    libhttppp::HTTPException err;
                    err[libhttppp::HTTPException::Error]  << "Cannot load symbol create: " << dlsym_error << '\n';
                    throw err;
                }


                dlerror();

                PluginApi *plg=create_plugin();
                plg->setArgs(args);
                plg->initPlugin();
                ldplg->ins=plg;

                if(_firstPlugin){
                    _lastPluging->next=ldplg;
                    _lastPluging=_lastPluging->next;
                }else{
                    _firstPlugin=ldplg;
                    _lastPluging=_firstPlugin;
                }
            }catch(libhttppp::HTTPException &e){
                std::cerr << e.what() << std::endl;
            }
        }
        closedir(directory);
    }
}

blogi::Plugin::~Plugin(){
    delete _firstPlugin;
}

blogi::Plugin::PluginData * blogi::Plugin::getFirstPlugin(){
    return _firstPlugin;
}

blogi::Plugin::PluginData::~PluginData(){
        const char* dlsym_error = dlerror();
        destroy_t* destroy_plugin = (destroy_t*) dlsym(pldata, "destroy");
        dlsym_error = dlerror();
        if (dlsym_error) {
            std::cerr << "Cannot load symbol destroy: " << dlsym_error << '\n';;
        }else{
            destroy_plugin(ins);
            dlclose(pldata);
        }
        delete next;
}

blogi::Plugin::PluginData::PluginData(){
    next=nullptr;
    ins=nullptr;
}

blogi::PluginApi * blogi::Plugin::PluginData::getInstace(){
    return ins;
}

blogi::Plugin::PluginData * blogi::Plugin::PluginData::getNextPlg(){
    return next;
}
