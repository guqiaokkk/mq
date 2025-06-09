#ifndef __M_BROKER_H__
#define __M_BROKER_H__

#include "muduo/proto/codec.h"
#include "muduo/proto/dispatcher.h"
#include "muduo/base/Logging.h"
#include "muduo/base/Mutex.h"
#include "muduo/net/EventLoop.h"
#include "muduo/net/TcpServer.h"
#include "../mqcommon/threadpool.hpp"
#include "../mqcommon/mq_msg.pb.h"
#include "../mqcommon/mq_proto.pb.h"
#include "../mqcommon/mq_logger.hpp"
#include "mq_connection.hpp"
#include "mq_consumer.hpp"
#include "mq_host.hpp"

namespace kjymq
{   
    #define DBFILE "/meta.db"
    #define HOSTNAME "kkkjyhost"
    class Server
    {
        public:
            typedef std::shared_ptr<google::protobuf::Message> MessagePtr;
            Server(int port, const std::string &basedir)
            :_server(&_baseloop, muduo::net::InetAddress("0.0.0.0", port),
                    "Server", muduo::net::TcpServer::kReusePort),
            _dispatcher(std::bind(&Server::onUnknownMessage, this, 
                        std::placeholders::_1,std::placeholders::_2, std::placeholders::_3)),
            _codec(std::make_shared<ProtobufCodec>(std::bind(&ProtobufDispatcher::onProtobufMessage,
                   &_dispatcher,std::placeholders::_1, std::placeholders::_2, std::placeholders::_3))),
            _virtual_host(std::make_shared<VirtualHost>(HOSTNAME, basedir, basedir + DBFILE)),
            _consumer_manager(std::make_shared<ConsumerManager>()),
            _connection_manager(std::make_shared<ConnectionManager>()),   
            _threadpool(std::make_shared<threadpool>())
            {
                //针对历史消息中的所有队列，初始化队列的消费者管理结构
                QueueMap qm = _virtual_host->allQueues();
                for(auto &q : qm)
                {
                    _consumer_manager->initQueueConsumer(q.first);
                }

             //注册业务请求处理函数
                _dispatcher.registerMessageCallback<kjymq::openChannelRequest>(std::bind(&Server::onOpenChannel, this, 
                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
                _dispatcher.registerMessageCallback<kjymq::closeChannelRequest>(std::bind(&Server::onCloseChannel, this, 
                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
                _dispatcher.registerMessageCallback<kjymq::declareExchangeRequest>(std::bind(&Server::onDeclareExchange, this, 
                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
                _dispatcher.registerMessageCallback<kjymq::deleteExchangeRequest>(std::bind(&Server::onDeleteExchange, this, 
                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
                _dispatcher.registerMessageCallback<kjymq::declareQueueRequest>(std::bind(&Server::onDeclareQueue, this, 
                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
                _dispatcher.registerMessageCallback<kjymq::deleteQueueRequest>(std::bind(&Server::onDeleteQueue, this, 
                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
                _dispatcher.registerMessageCallback<kjymq::queueBindRequest>(std::bind(&Server::onQueueBind, this, 
                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
                _dispatcher.registerMessageCallback<kjymq::queueUnBindRequest>(std::bind(&Server::onQueueUnBind, this, 
                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
                _dispatcher.registerMessageCallback<kjymq::basicPublishRequest>(std::bind(&Server::onBasicPublish, this, 
                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
                _dispatcher.registerMessageCallback<kjymq::basicAckRequest>(std::bind(&Server::onBasicAck, this, 
                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
                _dispatcher.registerMessageCallback<kjymq::basicConsumeRequest>(std::bind(&Server::onBasicConsume, this, 
                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
                _dispatcher.registerMessageCallback<kjymq::basicCancelRequest>(std::bind(&Server::onBasicCancel, this, 
                    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

                _server.setMessageCallback(std::bind(&ProtobufCodec::onMessage, _codec.get(),
                                           std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
                _server.setConnectionCallback(std::bind(&Server::onConnection, this, std::placeholders::_1));
            }

            void start()
            {
                _server.start();
                _baseloop.loop();
            }


        private:
            //打开信道 
            void onOpenChannel(const muduo::net::TcpConnectionPtr &conn, const openChannelRequestPtr &req, muduo::Timestamp)
            {
                Connection::ptr myconn = _connection_manager->getConnection(conn);
                if(myconn.get() == nullptr)
                {
                    DLOG("打开信道时，没有找到连接对应的Connection对象！");
                    conn->shutdown();
                    return;
                }
                return myconn->openChannel(req);
            }           
            //关闭信道
            void onCloseChannel(const muduo::net::TcpConnectionPtr &conn, const closeChannelRequestPtr &req, muduo::Timestamp)
            {
                Connection::ptr myconn = _connection_manager->getConnection(conn);
                if (myconn.get() == nullptr) {
                    DLOG("关闭信道时，没有找到连接对应的Connection对象！");
                    conn->shutdown();
                    return;
                }
                return myconn->closeChannel(req); 
            }

            //声明交换机
            void onDeclareExchange(const muduo::net::TcpConnectionPtr &conn, const declareExchangeRequestPtr &req, muduo::Timestamp)
            {
                Connection::ptr myconn = _connection_manager->getConnection(conn);
                if (myconn.get() == nullptr) {
                    DLOG("声明交换机时，没有找到连接对应的Connection对象！");
                    conn->shutdown();
                    return;
                }
                Channel::ptr cp = myconn->getChannel(req->cid());
                if (cp.get() == nullptr) {
                    DLOG("声明交换机时，没有找到信道！");
                    return;
                }
                return cp->declareExchange(req);
            }
            //删除交换机
            void onDeleteExchange(const muduo::net::TcpConnectionPtr &conn, const deleteExchangeRequestPtr &req, muduo::Timestamp)
            {
                Connection::ptr myconn = _connection_manager->getConnection(conn);
                if (myconn.get() == nullptr) {
                    DLOG("删除交换机时，没有找到连接对应的Connection对象！");
                    conn->shutdown();
                    return;
                }
                Channel::ptr cp = myconn->getChannel(req->cid());
                if (cp.get() == nullptr) {
                    DLOG("删除交换机时，没有找到信道！");
                    return;
                }
                return cp->deleteExchange(req);
            }

            //声明队列
            void onDeclareQueue(const muduo::net::TcpConnectionPtr &conn, const declareQueueRequestPtr &req, muduo::Timestamp)
            {
                Connection::ptr myconn = _connection_manager->getConnection(conn);
                if (myconn.get() == nullptr) {
                    DLOG("声明队列时，没有找到连接对应的Connection对象！");
                    conn->shutdown();
                    return;
                }
                Channel::ptr cp = myconn->getChannel(req->cid());
                if(cp.get() == nullptr)
                {
                    DLOG("声明队列时，没有找到信道！");
                    return;
                }
                return cp->declareQueue(req);
            }
            //删除队列
            void onDeleteQueue(const muduo::net::TcpConnectionPtr &conn, const deleteQueueRequestPtr &req, muduo::Timestamp)
            {
                Connection::ptr myconn = _connection_manager->getConnection(conn);
                if (myconn.get() == nullptr) {
                    DLOG("删除队列时，没有找到连接对应的Connection对象！");
                    conn->shutdown();
                    return;
                }
                Channel::ptr cp = myconn->getChannel(req->cid());
                if(cp.get() == nullptr)
                {
                    DLOG("删除队列时，没有找到信道！");
                    return;
                }
                return cp->deleteQueue(req);
            }

            //队列绑定
            void onQueueBind(const muduo::net::TcpConnectionPtr &conn, const queueBindRequestPtr &req, muduo::Timestamp)
            {
                Connection::ptr myconn = _connection_manager->getConnection(conn);
                if (myconn.get() == nullptr) {
                    DLOG("队列绑定时，没有找到连接对应的Connection对象！");
                    conn->shutdown();
                    return;
                }
                Channel::ptr cp = myconn->getChannel(req->cid());
                if (cp.get() == nullptr) {
                    DLOG("队列绑定时，没有找到信道！");
                    return;
                }
                return cp->queueBind(req);
            }
            //队列解绑
            void onQueueUnBind(const muduo::net::TcpConnectionPtr &conn, const queueUnBindRequestPtr &req, muduo::Timestamp)
            {
                Connection::ptr myconn = _connection_manager->getConnection(conn);
                if (myconn.get() == nullptr) {
                    DLOG("队列解绑时，没有找到连接对应的Connection对象！");
                    conn->shutdown();
                    return;
                }
                Channel::ptr cp = myconn->getChannel(req->cid());
                if (cp.get() == nullptr) {
                    DLOG("队列解绑时，没有找到信道！");
                    return;
                }
                return cp->queueUnBind(req);
            }

            //消息发布
            void onBasicPublish(const muduo::net::TcpConnectionPtr &conn, const basicPublishRequestPtr &req, muduo::Timestamp)
            {
                Connection::ptr myconn = _connection_manager->getConnection(conn);
                if (myconn.get() == nullptr) {
                    DLOG("发布消息时，没有找到连接对应的Connection对象！");
                    conn->shutdown();
                    return;
                }
                Channel::ptr cp = myconn->getChannel(req->cid());
                if (cp.get() == nullptr) {
                    DLOG("发布消息时，没有找到信道！");
                    return;
                }
                return cp->basicPublish(req);
            }
            //消息确认
            void onBasicAck(const muduo::net::TcpConnectionPtr& conn, const basicAckRequestPtr& req, muduo::Timestamp) {
                Connection::ptr myconn = _connection_manager->getConnection(conn);
                if (myconn.get() == nullptr) {
                    DLOG("确认消息时，没有找到连接对应的Connection对象！");
                    conn->shutdown();
                    return;
                }
                Channel::ptr cp = myconn->getChannel(req->cid());
                if (cp.get() == nullptr) {
                    DLOG("确认消息时，没有找到信道！");
                    return;
                }
                return cp->basicAck(req);
            }
            //队列消息订阅
            void onBasicConsume(const muduo::net::TcpConnectionPtr& conn, const basicConsumeRequestPtr& req, muduo::Timestamp) {
                Connection::ptr myconn = _connection_manager->getConnection(conn);
                if (myconn.get() == nullptr) {
                    DLOG("队列消息订阅时，没有找到连接对应的Connection对象！");
                    conn->shutdown();
                    return;
                }
                Channel::ptr cp = myconn->getChannel(req->cid());
                if (cp.get() == nullptr) {
                    DLOG("队列消息订阅时，没有找到信道！");
                    return;
                }
                return cp->basicConsume(req);
            }
            //队列消息取消订阅
            void onBasicCancel(const muduo::net::TcpConnectionPtr& conn, const basicCancelRequestPtr& req, muduo::Timestamp) {
                Connection::ptr myconn = _connection_manager->getConnection(conn);
                if (myconn.get() == nullptr) {
                    DLOG("队列消息取消订阅时，没有找到连接对应的Connection对象！");
                    conn->shutdown();
                    return;
                }
                Channel::ptr cp = myconn->getChannel(req->cid());
                if (cp.get() == nullptr) {
                    DLOG("队列消息取消订阅时，没有找到信道！");
                    return;
                }
                return cp->basicCancel(req);
            }
            
            void onUnknownMessage(const muduo::net::TcpConnectionPtr& conn, const MessagePtr& message, muduo::Timestamp) {
                LOG_INFO << "onUnknownMessage: " << message->GetTypeName();
                conn->shutdown();
            }

            void onConnection(const muduo::net::TcpConnectionPtr &conn)
            {
                if(conn->connected())
                {
                    _connection_manager->newConnection(conn, _codec, _consumer_manager, _virtual_host, _threadpool);
                }
                else
                {
                    _connection_manager->delConnection(conn);
                }
            }


        private:
            muduo::net::EventLoop _baseloop;
            muduo::net::TcpServer _server;//服务器对象
            ProtobufDispatcher _dispatcher;//请求分发器对象--要向其中注册请求处理函数
            ProtobufCodecPtr _codec;//protobuf协议处理器--针对收到的请求数据进行protobuf协议处理
            VirtualHost::ptr _virtual_host;
            ConsumerManager::ptr _consumer_manager;
            ConnectionManager::ptr _connection_manager;
            threadpool::ptr _threadpool;
    };


}

#endif