#include <iostream>
#include <thread>
#include <future>
#include <chrono>

int Add(int num1, int num2)
{
    std::cout << "加法！！1111\n";
    std::this_thread::sleep_for(std::chrono::seconds(5));
    std::cout << "AAADDDDDDD!" << std::endl;
    return num1 + num2;
}


int main()
{
    //std::async(func, ...)      std::async(policy, func, ...)
    //std::launch::deferred 在执行get获取异步结果的时候，才会执行异步任务
    //std::launch::async    内部会创建工作线程，异步的完成任务
    std::cout << "---------------------------1--------------------------------" << std::endl;
    std::future<int> result = std::async(std::launch::async, Add, 11, 22);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "---------------------------2--------------------------------" << std::endl;
    int sum = result.get();
    //等待异步任务执行完毕，会阻塞等待

    std::cout << "---------------------------3--------------------------------" << std::endl;
    std::cout << sum << std::endl;
}