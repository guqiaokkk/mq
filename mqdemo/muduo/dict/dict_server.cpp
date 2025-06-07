#include "../include/muduo/net/TcpServer.h"
#include "../include/muduo/net/EventLoop.h"
#include "../include/muduo/net/TcpConnection.h"

#include <iostream>
#include <unordered_map>
#include <functional>


class TranslateServer 
{
public:
    TranslateServer(int port):_server(&_baseloop,
    muduo::net::InetAddress("0.0.0.0", port),
"TranslateServer", muduo::net::TcpServer::kReusePort)
{
    //将我们的类成员函数，设置为服务器的回调处理函数
    //因为_server的函数的参数只要一个参数，所以我们bind

    _server.setConnectionCallback(std::bind(&TranslateServer::onConnection, this, std::placeholders::_1));
    _server.setMessageCallback(std::bind(&TranslateServer::onMessage, this, std::placeholders::_1,
    std::placeholders::_2,std::placeholders::_3));
}

//启动服务器
void Start()
{
    _server.start();//开始事件监听
    _baseloop.loop();//开始事件监控，这是一个死循环阻塞接口
}


private:
    //onConnection 在一个链接建立成功，以及关闭时被调用
    void onConnection(const muduo::net::TcpConnectionPtr &conn)
    {
        if(conn->connected() == true){
            std::cout << "新链接成功 " << std::endl;
        }
        else{
            std::cout << "新链接关闭 " << std::endl;
        }
    }

    std::string translate(const std::string &str)
    {
        //新连接建立成功时的回调函数
        static std::unordered_map<std::string, std::string> dict_map = {
            {"hello","你好"},
            {"Hello","你好"},
            {"nihao","你好"}
        };
        auto it = dict_map.find(str);
        if(it == dict_map.end()){
            return "UnFind";
        }
        return it->second;
    }

    //通信连接收到请求时的回调函数
    void onMessage(const muduo::net::TcpConnectionPtr &conn, muduo::net::Buffer *buff, muduo::Timestamp)
    {
        //1.从buff中把请求的数据提取出来
        std::string tmp = buff->retrieveAllAsString();
        //2.调用translate接口进行翻译    
        std::string resp = translate(tmp);
        //3.对客户端响应结果
        conn->send(resp);
    }


private:
//_baseloop是epoll的事件监控，会进行描述符的事件监控，触发事件后进行io操作
    muduo::net::EventLoop _baseloop;
     //这个server对象，主要用于设置回调函数，用于告诉服务器收到什么请求该如何处理
    muduo::net::TcpServer _server;
};




int main()
{
    TranslateServer server(8085);
    server.Start();
    return 0;
}
