#include <ctime>
#include <iostream>

//封装一个日志宏，通过日志宏进行日志的打印，在打印的信息前带有系统时间以及文件名和行号
//     [17:26:24] [log.cpp:12] 打开文件失败！ 

#define DBG_level 0
#define WAR_level 1
#define ERR_level 2
#define DEFAULT_level DBG_level


#define LOG(level_str, level, format, ...){\
    if(level >= DBG_level){\
        time_t t = time(NULL);\
        struct tm *nowtime = localtime(&t);\
        char time_str[32];\
        strftime(time_str, 31, "%H:%M:%S", nowtime);\
        printf("[%s][%s]:[%s:%d]\t" format "\n", level_str, time_str, __FILE__, __LINE__, ##__VA_ARGS__);\
    }\
}

#define DLOG(format, ...) LOG("DBG", DBG_level,format,  ##__VA_ARGS__)
#define WLOG(format, ...) LOG("WAR", WAR_level,format,  ##__VA_ARGS__)
#define ELOG(format, ...) LOG("ERR", ERR_level,format,  ##__VA_ARGS__)//##_VA_ARGS_用于处理可变参数



int main()
{
    DLOG("hello world");
    WLOG("hello worlddada");
    ELOG();

    return 0;
}


