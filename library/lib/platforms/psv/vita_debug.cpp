//
// Created by fang on 2022/11/26.
//

#include <cstdio>
#include <cstdarg>
#include <ctime>

#define DEBUG_PATH "ux0:data/borealis.txt"

FILE *debug_stream = nullptr;

int debug(const char *format, ...){
    if(debug_stream == nullptr) return -1;
    va_list va_ptr;
    va_start(va_ptr, format);

    vfprintf(debug_stream,format,va_ptr);

    va_end(va_ptr);
    fflush(debug_stream);
    return 0;
}

int cleanupDebug(){
    if(debug_stream == nullptr) return -1;
    fclose(debug_stream);
    debug_stream = nullptr;
    return 0;
}

void outputTime(){
    time_t rawtime;
    struct tm * timeinfo;
    time ( &rawtime );
    timeinfo = localtime ( &rawtime );
    debug( "time: %s\n", asctime (timeinfo) );
}

int startDebug(){
    if(debug_stream != nullptr) return -1;
    debug_stream = fopen(DEBUG_PATH, "w");
    debug("============== start debug \n");
    outputTime();
    return 0;
}