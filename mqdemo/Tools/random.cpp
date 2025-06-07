#include <iostream>
#include <random>
#include <sstream>
#include <iomanip>
#include <atomic>

static std::string UUID()
{
    std::random_device rd;
    // size_t num = rd();//生成一个机器随机数，效率太低
    //因此解决方案，就是通过一个机器随机数，作为生成伪随机数的种子
    std::mt19937_64 gernator(rd());//通过梅森旋转算法，生成一个伪随机数
    //我们要生成的是8个0~255的数字，所以要限定数字区间
    std::uniform_int_distribution<int> distribution(0,255);//[0,255]
    //然后将生成的数字转换为16进制的数字字符
    std::stringstream ss;//字符流
    for(int i = 0; i < 8; i++){
        ss << std::setw(2) << std::setfill('0') << std::hex << distribution(gernator);//std::hex将后续的整数输出格式设置为十六进制。
        if(i == 3 || i == 5 || i == 7){
            ss << "-";
        }
    }

    static std::atomic<size_t> seq(1);//定义一个原子类型整数，初始化为1. 这里定义为static是因为我们要把这段代码封装成一个函数，所以用全局变量
    size_t num = seq.fetch_add(1);
    for(int i = 7; i >= 0; i--)
    {
        ss << std::setw(2) << std::setfill('0') << std::hex << ((num >> (i * 8)) & 0xff);
        if(i == 6) ss << "-";
    }
    return ss.str();
}









int main()
{
    // std::random_device rd;
    // // size_t num = rd();//生成一个机器随机数，效率太低
    // //因此解决方案，就是通过一个机器随机数，作为生成伪随机数的种子
    // std::mt19937_64 gernator(rd());//通过梅森旋转算法，生成一个伪随机数
    // //我们要生成的是8个0~255的数字，所以要限定数字区间
    // std::uniform_int_distribution<int> distribution(0,255);//[0,255]
    // //然后将生成的数字转换为16进制的数字字符
    // std::stringstream ss;//字符流
    // for(int i = 0; i < 8; i++){
    //     ss << std::setw(2) << std::setfill('0') << std::hex << distribution(gernator);//std::hex将后续的整数输出格式设置为十六进制。
    //     if(i == 3 || i == 5 || i == 7){
    //         ss << "-";
    //     }
    // }

    // static std::atomic<size_t> seq(1);//定义一个原子类型整数，初始化为1. 这里定义为static是因为我们要把这段代码封装成一个函数，所以用全局变量
    // size_t num = seq.fetch_add(1);
    // for(int i = 7; i >= 0; i--)
    // {
    //     ss << std::setw(2) << std::setfill('0') << std::hex << ((num >> (i * 8)) & 0xff);
    //     if(i == 6) ss << "-";
    // }
    // std::cout << ss.str() << std::endl;
    for(int i = 0; i < 20; i++)
    {
        std::cout << UUID() << std::endl;
    }
    return 0;
}