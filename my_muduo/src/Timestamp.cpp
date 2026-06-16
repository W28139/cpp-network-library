#include "my_muduo/Timestamp.h"
#include <ctime>

// 默认构造函数，用当前时间初始化对象
Timestamp::Timestamp() {
    secsinceepoch_ = time(nullptr);
}

// 用一个整数时间初始化对象
Timestamp::Timestamp(int64_t secsinceepoch) 
    : secsinceepoch_(secsinceepoch) 
{
}

// 静态方法：返回当前时间的 Timestamp 对象
Timestamp Timestamp::now() {
    return Timestamp(); // 调用默认构造函数获取当前时间
}

// 返回整数表示的时间
time_t Timestamp::toint() const {
    return secsinceepoch_;
}

// 返回字符串表示的时间 格式：yyyy-mm-dd hh24:mi:ss
std::string Timestamp::tostring() const {
    char buf[64] = {0};
    struct tm tm_time;
    
    // 使用线程安全的 localtime_r 将 time_t 转换为明文时间结构体 tm
    // localtime_r 是 Linux 下推荐的写法，防止多线程竞争静态缓冲区
    localtime_r(&secsinceepoch_, &tm_time);

    snprintf(buf, sizeof(buf), "%4d-%02d-%02d %02d:%02d:%02d",
             tm_time.tm_year + 1900, 
             tm_time.tm_mon + 1,    
             tm_time.tm_mday,
             tm_time.tm_hour,
             tm_time.tm_min,
             tm_time.tm_sec);
    return buf;
}
