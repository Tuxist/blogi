#include <algorithm>
#include <brotli/encode.h>

#pragma once

template<typename T>
void compress(const std::string in,std::string &out){

    size_t len= BrotliEncoderMaxCompressedSize(in.length());

    unsigned char *buf = new unsigned char[len];

    BrotliEncoderCompress(BROTLI_DEFAULT_QUALITY,BROTLI_DEFAULT_WINDOW,BROTLI_MODE_TEXT,
                      in.length(),(const unsigned char*)in.c_str(),&len,buf);

    out.resize(len);

    std::copy(buf,buf+len,std::inserter<std::string>(out,out.begin()));

    delete[] buf;
}
