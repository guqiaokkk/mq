#include "mq_channel.hpp"

namespace kjymq
{
    class Connection
    {
        public:
            using ptr = std::shared_ptr<Connection>;
            Connection(const muduo::net::TcpConnectionPtr &conn,
                       const ProtobufCodecPtr &codec,
                       const ConsumerManager::ptr &cmp,
                       const VirtualHost::ptr &host,
                       const threadpool::ptr &pool):
                       _conn(conn),
                       _codec(codec),
                       _cmp(cmp),
                       _host(host),
                       _pool(pool),
                       _channels(std::make_shared<ChannelManager>()){}
            
            //创建信道
            void openChannel(const openChannelRequestPtr &req)
            {
                //1. 判断信道ID是否重复,创建信道
                bool ret = _channels->openChannel(req->cid(), _conn, _codec, _cmp, _host, _pool);
                if(ret == false)
                {
                    DLOG("创建信道的时候，信道ID重复了");
                    return basicResponse(false, req->rid(), req->cid());
                } 
                DLOG("%s 信道创建成功！", req->cid().c_str());
                //2. 给客户端进行回复  
                return basicResponse(true, req->rid(), req->cid());
            }

            //关闭信道
            void closeChannel(const closeChannelRequestPtr &req)
            {
                _channels->closeChannel(req->cid());
                return basicResponse(true, req->rid(), req->cid());
            }

            //获取信道
            Channel::ptr getChannel(const std::string &cid)
            {
                return _channels->getChannel(cid);
            }

        private:
            void basicResponse(bool ok, const std::string &rid, const std::string &cid)
            {
                basicCommonResponse resp;
                resp.set_ok(ok);
                resp.set_rid(rid);
                resp.set_cid(cid);
                _codec->send(_conn, resp);
                return;
            }    

        private:
            muduo::net::TcpConnectionPtr _conn;
            ProtobufCodecPtr _codec;
            ConsumerManager::ptr _cmp;
            VirtualHost::ptr _host;
            threadpool::ptr _pool;
            ChannelManager::ptr _channels;
    };

    class ConnectionManager
    {
        public:
            using ptr = std::shared_ptr<ConnectionManager>;
            ConnectionManager(){}
            //创立新的connection
            void newConnection(const muduo::net::TcpConnectionPtr &conn,
                               const ProtobufCodecPtr &codec,
                               const ConsumerManager::ptr &cmp,
                               const VirtualHost::ptr &host,
                               const threadpool::ptr &pool)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                auto it = _conns.find(conn);
                if (it != _conns.end()) {
                    return ;
                }
                Connection::ptr myself_conn = std::make_shared<Connection>(conn, codec, cmp, host, pool);
                _conns.insert(std::make_pair(conn, myself_conn));
                return;
            }
            //删除connection
            void delConnection(const muduo::net::TcpConnectionPtr &conn)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _conns.erase(conn);
            }
            //获取一个connection
            Connection::ptr getConnection(const muduo::net::TcpConnectionPtr &conn)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                auto it = _conns.find(conn);
                if (it == _conns.end()) {
                    return Connection::ptr();
                }
                return it->second;
            }


        private:
            std::mutex _mutex;
            std::unordered_map<muduo::net::TcpConnectionPtr, Connection::ptr> _conns;
    };

}

