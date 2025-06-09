#ifndef __M_MSG_H__
#define __M_MSG_H__
#include "../mqcommon/mq_helper.hpp"
#include "../mqcommon/mq_msg.pb.h"
#include "../mqcommon/mq_logger.hpp"
#include <string>
#include <iostream>
#include <unordered_map>
#include <mutex>
#include <memory>
#include <list>

namespace kjymq{
    #define DATAFILE_SUBFIX ".mqd"
    #define TMPFILE_SUBFIX ".mqd.tmp"

    using MessagePtr = std::shared_ptr<Message>;

    //消息持久化存储管理器
    class MessageMapper
    {
        public:
            MessageMapper(std::string &basedir, const std::string qname):_qname(qname)
            {
                if(basedir.back() != '/')basedir += ('/');
                _datafile = basedir + qname + DATAFILE_SUBFIX;
                _tmpfile = basedir + qname + TMPFILE_SUBFIX;
                if(FileHelper(basedir).exist() == false){
                    assert(FileHelper::createDirectory(basedir));
                }
                createMsgFile();
            }

            //检查数据文件 _datafile 是否存在,不存在就创建
            bool createMsgFile()
            {
                if(FileHelper(_datafile).exist() == true)
                {
                    return true;
                }
                bool ret = FileHelper::createFile(_datafile);
                if(ret == false)
                {
                    DLOG("创建队列数据文件 %s 失败！", _datafile.c_str());
                    return false;
                }
                return true;
            }

            void removeMsgFile()
            {
                FileHelper::removeFile(_datafile);
                FileHelper::removeFile(_tmpfile);
            }

            bool insert(MessagePtr &msg)
            {
                return insert(_datafile, msg);
            }

            bool remove(MessagePtr &msg)
            {
                //1. 将msg中的有效标志位修改为 '0'
                msg->mutable_payload()->set_valid("0");
                //2. 对msg进行序列化
                std::string body = msg->payload().SerializeAsString();
                if(body.size() != msg->length())
                {
                    DLOG("不能修改文件中的数据信息，因为新生成的数据与原数据长度不一致!");
                    return false;
                }
                //3. 将序列化后的消息，写入到数据在文件中的指定位置（覆盖原有的数据）
                FileHelper helper(_datafile);
                bool ret = helper.write(body.c_str(), msg->offset(), body.size());
                if(ret == false)
                {
                    DLOG("向队列数据文件写入数据失败！");
                    return false;
                }
                return true;
            }

            std::list<MessagePtr> gc()
            {
                bool ret;
                std::list<MessagePtr> result;
                //1.加载有效数据
                ret = load(result);
                if(ret == false)
                {
                    DLOG("加载有效数据失败！\n");
                    return result;
                }
                //2. 将有效数据，进行序列化存储到临时文件中
                FileHelper::createFile(_tmpfile);
                for(auto &msg : result)
                {
                    DLOG("向临时文件写入数据: %s", msg->payload().body().c_str());
                    ret = insert(_tmpfile, msg);
                    if(ret == false)
                    {
                        DLOG("向临时文件写入消息数据失败！！");
                        return result;
                    }
                }
                //3. 删除源文件
                ret = FileHelper::removeFile(_datafile);
                if (ret == false) {
                    DLOG("删除源文件失败！");
                    return result;
                }
                //4. 修改临时文件名，为源文件名称
                ret = FileHelper(_tmpfile).rename(_datafile);
                if (ret == false) {
                    DLOG("修改临时文件名称失败！");
                    return result;
                }
                //5. 返回新的有效数据
                return result;
            }


        private:
            bool load(std::list<MessagePtr> &result)
            {
                //1. 加载出文件中所有的有效数据；  存储格式 4字节长度|数据|4字节长度|数据.....
                FileHelper data_file_helper(_datafile);
                size_t offset = 0, msg_size;
                size_t fsize = data_file_helper.size();
                bool ret;
                while(offset < fsize)
                {
                    ret = data_file_helper.read((char*)&msg_size, offset, sizeof(size_t));
                    if(ret == false)
                    {
                        DLOG("读取消息长度失败！");
                        return false;
                    }
                    offset += sizeof(size_t);
                    std::string msg_body(msg_size, '\0');
                    ret = data_file_helper.read(&msg_body[0], offset, msg_size);
                    if(ret == false)
                    {
                        DLOG("读取消息数据失败！");
                        return false;
                    }
                    offset += msg_size;

                    MessagePtr mp = std::make_shared<Message>();
                    //调用 ParseFromString 方法，将字符串 msg_body 解析为 Payload 对象
                    mp->mutable_payload()->ParseFromString(msg_body);
                    //如果是无效消息，则直接处理下一个
                    if(mp->payload().valid() == "0")
                    {
                        DLOG("加载到无效信息 %s", mp->payload().body().c_str());
                        continue;
                    }
                    //有效消息则保存起来
                    result.push_back(mp);
                }
                return true;
            }

            bool insert(const std::string &filename, MessagePtr &msg)
            {
                //新增数据都是添加在文件末尾的
                //1. 进行消息的序列化，获取到格式化后的消息
                std::string body = msg->payload().SerializeAsString();
                //2. 获取文件长度
                FileHelper helper(filename);
                size_t fsize = helper.size();
                size_t msg_size = body.size();
                //写入逻辑：1. 先写入4字节数据长度， 2， 再写入指定长度数据
                
                //在文件末尾写入消息体的长度，以便后续可以正确读取消息体。
                bool ret = helper.write((char*)&msg_size, fsize, sizeof(size_t));
                if (ret == false) {
                    DLOG("向队列数据文件写入数据长度失败！");
                    return false;
                }
                //3. 将数据写入文件的指定位置
                ret = helper.write(body.c_str(), fsize + sizeof(size_t), body.size());
                if (ret == false) {
                    DLOG("向队列数据文件写入数据失败！");
                    return false;
                }
                //4. 更新msg中的实际存储信息
                msg->set_offset(fsize + sizeof(size_t));
                msg->set_length(body.size());
                return true;
            }

        private:
            std::string _qname;// 队列名称
            std::string _datafile;// 存储消息数据的文件名
            std::string _tmpfile;// 临时文件名，用于垃圾回收时暂存有效数据
    };


    class QueueMessage
    {
        public:
            using ptr = std::shared_ptr<QueueMessage>;
            QueueMessage(std::string &basedir, const std::string qname):_mapper(basedir, qname),
            _qname(qname),_valid_count(0),_total_count(0){}
            
            bool recovery()
            {
                //恢复历史消息
                std::unique_lock<std::mutex> lock(_mutex);
                _msgp = _mapper.gc();
                for(auto &msg : _msgp)
                {
                    _durable_hash.insert(std::make_pair(msg->payload().properties().id(), msg));
                }
                _valid_count = _total_count = _msgp.size();
                return true;
            }



            bool insert(const BasicProperties *bp, const std::string &body, bool queue_is_durable)
            {
                //1. 构造消息对象
                MessagePtr msg = std::make_shared<Message>();
                msg->mutable_payload()->set_body(body);
                if(bp != nullptr)
                {
                    DeliveryMode mode = queue_is_durable ? bp->delivery_mode() : DeliveryMode::UNDURABLE;
                    msg->mutable_payload()->mutable_properties()->set_id(bp->id());
                    msg->mutable_payload()->mutable_properties()->set_delivery_mode(mode);
                    msg->mutable_payload()->mutable_properties()->set_routing_key(bp->routing_key());
                }
                else
                {
                    DeliveryMode mode = queue_is_durable ?  DeliveryMode::DURABLE : DeliveryMode::UNDURABLE;
                    msg->mutable_payload()->mutable_properties()->set_id(UUIDhelper::UUID());
                    msg->mutable_payload()->mutable_properties()->set_delivery_mode(mode);
                    msg->mutable_payload()->mutable_properties()->set_routing_key("");
                }
                std::unique_lock<std::mutex> lock(_mutex);
                //2. 判断消息是否需要持久化
                if(msg->mutable_payload()->mutable_properties()->delivery_mode() == DeliveryMode::DURABLE)
                {
                    //在持久化存储中表示数据有效
                    msg->mutable_payload()->set_valid("1");
                    //3. 进行持久化存储
                    bool ret = _mapper.insert(msg);
                    if (ret == false) {
                        DLOG("持久化存储消息：%s 失败了！", body.c_str());
                        return false;
                    }
                    _valid_count += 1;
                    _total_count += 1;
                    _durable_hash.insert(std::make_pair(msg->payload().properties().id(), msg));
                }
                //4. 内存的管理
                _msgp.push_back(msg);
                return true;
            }
            
            bool remove(const std::string &msg_id)//每次删除消息后，判断是否要垃圾回收
            {
                std::unique_lock<std::mutex> lock(_mutex);
                //1. 从待确认队列中查找消息
                auto it = _waitcheck_hash.find(msg_id);
                if(it == _waitcheck_hash.end())
                {
                    DLOG("没有找到要删除的消息：%s!", msg_id.c_str());
                    return true;
                }
                //2. 根据消息的持久化模式，决定是否删除持久化信息
                if(it->second->payload().properties().delivery_mode() == DeliveryMode::DURABLE)
                {
                    //3. 删除持久化信息
                    _mapper.remove(it->second);
                    _durable_hash.erase(msg_id);
                    _valid_count -= 1;//持久化文件中有效消息数量 -1
                    msggc();//内部判断是否需要垃圾回收，需要的话则回收一下
                }
                //4. 删除内存中的信息
                _waitcheck_hash.erase(msg_id);
                return true;
            }

            MessagePtr Front()
            {
                std::unique_lock<std::mutex> lock(_mutex);
                if(_msgp.size() == 0)
                {
                    return MessagePtr();
                }
                //获取一条队首消息：从_msgp中取出数据
                MessagePtr tmp = _msgp.front();
                _msgp.pop_front();
                //将该消息对象，向待确认的hash表中添加一份，等到收到消息确认后进行删除
                _waitcheck_hash.insert(std::make_pair(tmp->payload().properties().id(), tmp));
                return tmp;
            }

            size_t getable_count()
            {
                std::unique_lock<std::mutex> lock(_mutex);
                return _msgp.size();
            }
            size_t total_count()
            {
                std::unique_lock<std::mutex> lock(_mutex);
                return _total_count;
            }
            size_t durable_count()
            {
                std::unique_lock<std::mutex> lock(_mutex);
                return _durable_hash.size();
            }
            size_t waitack_count()
            {
                std::unique_lock<std::mutex> lock(_mutex);
                return _waitcheck_hash.size();
            }

            void clear()            
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _mapper.removeMsgFile();
                _durable_hash.clear();
                _waitcheck_hash.clear();
                _msgp.clear();
                _valid_count = _total_count = 0;
            }

        private:
            bool gccheck()
            {
                if(_total_count > 2000 && _valid_count * 10 / _total_count < 5)
                {
                    return true;
                }
                return false;
            }

            void msggc()
            {
                //1. 进行垃圾回收，获取到垃圾回收后，有效的消息信息链表
                if(gccheck() == false)return;
                std::list<MessagePtr> msgp = _mapper.gc();
                for(auto &msg : msgp)
                {
                    auto it = _durable_hash.find(msg->payload().properties().id());
                    if(it == _durable_hash.end())
                    {
                        DLOG("垃圾回收后，有一条持久化消息，在内存中没有进行管理!");
                        _msgp.push_back(msg); //做法：重新添加到推送链表的末尾
                        _durable_hash.insert(std::make_pair(msg->payload().properties().id(), msg));
                        continue;
                    }
                    //2. 更新每一条消息的实际存储位置
                    it->second->set_offset(msg->offset());
                    it->second->set_length(msg->length());
                }
                //3. 更新当前的有效消息数量 & 总的持久化消息数量
                _valid_count = _total_count = msgp.size();
            }


        private:
            std::mutex _mutex;
            std::string _qname;
            size_t _valid_count;
            size_t _total_count;
            MessageMapper _mapper;
            std::list<MessagePtr> _msgp;//待推送消息链表
            std::unordered_map<std::string, MessagePtr> _durable_hash;//持久化消息hash
            std::unordered_map<std::string, MessagePtr> _waitcheck_hash;//待确认消息hash
    };



    class MessageManager
    {
        public:
            using ptr = std::shared_ptr<MessageManager>;
            MessageManager(const std::string &basedir):_basedir(basedir){}

            void clear()
            {
                std::unique_lock<std::mutex> lock(_mutex);
                for(auto &msgp : _queue_msgp)
                {
                    msgp.second->clear();
                }
            }

            //初始化一个消息队列（如果不存在则创建）
            void InitQueueMessage(const std::string &qname)
            {
                QueueMessage::ptr tmp;
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it = _queue_msgp.find(qname);
                    if(it != _queue_msgp.end())//此时表示qname对应的队列管理存在，返回
                    {
                        return;
                    }
                    tmp = std::make_shared<QueueMessage>(_basedir, qname);
                    _queue_msgp.insert(std::make_pair(qname, tmp));
                }
                tmp->recovery();
            }

            //销毁一个消息队列，并清理其数据。
            void destroyQueueMessage(const std::string &qname)
            {
                QueueMessage::ptr tmp;
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it = _queue_msgp.find(qname);
                    if(it == _queue_msgp.end())
                    {
                        return;
                    }
                    tmp = it->second;
                    _queue_msgp.erase(it);
                }
                tmp->clear();
            }
            
            // 因为视频这里insert的最后一个参数还不是bool类型的，你现在是bool类型的，导致上层传入的kjymq::DeliveryMode::DURABLE kjymq::DeliveryMode::UNDURABLE
            // 都是真的，所以都会进行持久化   你现在写的insert是最终版的insert会修改为bool类型，但是前期测试我记得还是枚举

            //向指定队列插入一条消息。
            bool insert(const std::string &qname, BasicProperties *bp, const std::string &body, bool queue_is_durable)
            {
                QueueMessage::ptr tmp;
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it = _queue_msgp.find(qname);
                    if(it == _queue_msgp.end())
                    {
                        DLOG("向队列%s新增消息失败:没有找到消息管理句柄!", qname.c_str());
                        return false;
                    }
                    tmp = it->second;
                }
                return tmp->insert(bp, body, queue_is_durable);
            }

            //获取队列的队首消息（用于消费者读取）
            MessagePtr Front(const std::string &qname)
            {
                QueueMessage::ptr tmp;
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it = _queue_msgp.find(qname);
                    if(it == _queue_msgp.end())
                    {
                        DLOG("获取队列%s队首消息失败:没有找到消息管理句柄!", qname.c_str());
                        return MessagePtr();
                    }
                    tmp = it->second;
                }
                return tmp->Front();
            }

            //确认队列中的一条消息已经被成功处理，并将其从队列中删除
            void ack(const std::string &qname, const std::string &msg_id)
            {
                QueueMessage::ptr tmp;
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it = _queue_msgp.find(qname);
                    if(it == _queue_msgp.end())
                    {
                        DLOG("确认队列%s消息%s失败:没有找到消息管理句柄!", qname.c_str(), msg_id.c_str());
                        return;
                    }
                    tmp = it->second;
                }
                tmp->remove(msg_id);
                return;
            }

            size_t getable_count(const std::string &qname)
            {
                QueueMessage::ptr tmp;
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it = _queue_msgp.find(qname);
                    if(it == _queue_msgp.end())
                    {
                        DLOG("获取队列%s待推送消息数量失败:没有找到消息管理句柄!", qname.c_str());
                        return 0;
                    }
                    tmp = it->second;
                }
                return tmp->getable_count();
            }

            size_t durable_count(const std::string &qname)
            {
                QueueMessage::ptr tmp;
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it = _queue_msgp.find(qname);
                    if(it == _queue_msgp.end())
                    {
                        DLOG("获取队列%s有效持久化消息数量失败:没有找到消息管理句柄!", qname.c_str());
                        return 0;
                    }
                    tmp = it->second;
                }
                return tmp->durable_count();
            }

            size_t waitack_count(const std::string &qname)
            {
                QueueMessage::ptr tmp;
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it = _queue_msgp.find(qname);
                    if(it == _queue_msgp.end())
                    {
                        DLOG("获取队列%s待确认消息数量失败:没有找到消息管理句柄!", qname.c_str());
                        return 0;
                    }
                    tmp = it->second;
                }
                return tmp->waitack_count();
            }

            size_t total_count(const std::string &qname)
            {
                QueueMessage::ptr tmp;
                {
                    std::unique_lock<std::mutex> lock(_mutex);
                    auto it = _queue_msgp.find(qname);
                    if(it == _queue_msgp.end())
                    {
                        DLOG("获取队列%s总持久化消息数量失败:没有找到消息管理句柄!", qname.c_str());
                        return 0;
                    }
                    tmp = it->second;
                }
                return tmp->total_count();
            }

        private:
            std::mutex _mutex;
            std::string _basedir;
            std::unordered_map<std::string, QueueMessage::ptr> _queue_msgp;// 队列名 -> 队列管理对象的映射
    };



}

#endif