#ifndef __M_HOST_H__
#define __M_HOST_H__

#include "mq_binding.hpp"
#include "mq_exchange.hpp"
#include "mq_message.hpp"
#include "mq_queue.hpp"

namespace kjymq{

    class VirtualHost{
        public:
            using ptr = std::shared_ptr<VirtualHost>;
            VirtualHost(const std::string &hname, const std::string &basedir, const std::string &dbfile):
            _host_name(hname),
            _emp(std::make_shared<ExchangeManager>(dbfile)),
            _mqmp(std::make_shared<MsgQueueManager>(dbfile)),
            _bmp(std::make_shared<BindingManager>(dbfile)),
            _mmp(std::make_shared<MessageManager>(basedir))
            {
                //获取到所有的队列信息，通过队列名称恢复历史消息数据
                QueueMap qm = _mqmp->allQueues();
                for(auto & q: qm)
                {
                    _mmp->InitQueueMessage(q.first);
                }
            }
        
        //声明（创建）一个交换机
        bool declareExchange(const std::string &name,
            ExchangeType type, 
            bool durable, 
            bool auto_delete,
            const google::protobuf::Map<std::string, std::string> &args)
        {
            return _emp->declareExchange(name, type, durable, auto_delete, args);
        }
        //删除交换机，并清理其所有绑定关系（避免脏数据）
        void deleteExchange(const std::string &name)
        {
            //删除交换机的时候，需要将交换机相关的绑定信息也删除掉。
            _bmp->removeExchangeBindings(name);
            _emp->deleteExchange(name);
            return;
        }
        //检查交换机是否存在
        bool existsExchange(const std::string &name)
        {
            return _emp->exists(name);
        }
        //获取交换机对象
        Exchange::ptr selectExchange(const std::string &ename)
        {
            return _emp->selectExchange(ename);
        }

        //声明（创建）一个队列，并初始化其消息存储
        bool declareQueue(const std::string &qname,
            bool qdurable,
            bool qexclusive,
            bool qauto_delete,
            const google::protobuf::Map<std::string, std::string> &qargs)
        {
            //初始化队列的消息句柄（消息的存储管理）
            _mmp->InitQueueMessage(qname);
            //队列的创建
            return _mqmp->declareQueue(qname, qdurable, qexclusive, qauto_delete, qargs);
        }
        //删除队列，并清理其消息和绑定关系
        void deleteQueue(const std::string &qname)
        {
            //删除的时候队列相关的数据有两个：队列的消息，队列的绑定信息
            _mmp->destroyQueueMessage(qname);
            _bmp->removeMsgQueueBindings(qname);
            return _mqmp->deleteQueue(qname);
        }   
        //检查队列是否存在
        bool existsQueue(const std::string &qname)
        {
            return _mqmp->exists(qname);
        }
        //获取所有队列信息
        QueueMap allQueues()
        {
            return _mqmp->allQueues();
        }

        //绑定交换机和队列（指定路由键 key）
        bool bind(const std::string &ename, const std::string &qname, const std::string &key)
        {
            Exchange::ptr ep = _emp->selectExchange(ename);
            if(ep.get() == nullptr)
            {
                DLOG("进行队列绑定失败，交换机%s不存在！", ename.c_str());
                return false;
            }

            Msgqueue::ptr mp = _mqmp->selectQueue(qname);
            if(mp.get() == nullptr)
            {
                DLOG("进行队列绑定失败，队列%s不存在！", qname.c_str());
                return false;
            }

            return _bmp->bind(ename, qname, key, ep->durable && mp->durable);
        }
        //解绑交换机和队列
        void unbind(const std::string &ename, const std::string &qname)
        {
            return _bmp->unBind(ename, qname);
        }
        //查询一个交换机所有的绑定信息
        MsgQueueBindMap exchangeBindings(const std::string &ename)
        {
            return _bmp->getExchangeBindings(ename);
        }
        //查询一个交换机和一个队列之间是否存在绑定
        bool existsBinding(const std::string &ename, const std::string &qname)
        {
            return _bmp->exists(ename, qname);
        }

        //发布消息到指定队列
        bool basicPublish(const std::string &qname, BasicProperties *bp, const std::string &body)
        {
            Msgqueue::ptr mp = _mqmp->selectQueue(qname);
            if(mp.get() == nullptr)
            {
                DLOG("发布消息失败，队列%s不存在！", qname.c_str());
                return false;
            }
            return _mmp->insert(qname, bp, body, mp->durable);
        }
        //从指定队列中获取一条消息
        MessagePtr basicConsume(const std::string &qname)
        {
            return _mmp->Front(qname);
        }
        //确认消息已被成功处理
        void basicack(const std::string &qname, const std::string &msg_id)
        {
            _mmp->ack(qname, msg_id);
            return;
        }
        //清空整个虚拟主机的数据
        void clear()
        {
            _emp->clear();
            _mqmp->clear();
            _bmp->clear();
            _mmp->clear();
        }


        private:
            std::string _host_name;
            ExchangeManager::ptr _emp;
            MsgQueueManager::ptr _mqmp;
            BindingManager::ptr _bmp;
            MessageManager::ptr _mmp;
    };

}





#endif