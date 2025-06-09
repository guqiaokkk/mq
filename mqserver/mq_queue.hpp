#ifndef __M_QUEUE_H__
#define __M_QUEUE_H__


#include "../mqcommon/mq_logger.hpp"
#include "../mqcommon/mq_helper.hpp"
#include "../mqcommon/mq_msg.pb.h"
#include <iostream>
#include <unordered_map>
#include <mutex>
#include <memory>


namespace kjymq{


    struct Msgqueue{
        using ptr = std::shared_ptr<Msgqueue>;

        std::string name;
        bool durable;
        //是否独占标识
        bool exclusive;
        bool auto_delete;
        google::protobuf::Map<std::string, std::string> args;//附加参数（键值对形式）


        Msgqueue(){}
        Msgqueue(std::string qname,
        bool qdurable, 
        bool qexclusive, 
        bool qauto_delete,
        const google::protobuf::Map<std::string, std::string> qargs):
        name(qname),
        durable(qdurable), 
        exclusive(qexclusive), 
        auto_delete(qauto_delete),
        args(qargs){}

        void setArgs(const std::string &str_args)
        {
            std::vector<std::string> substrs;
            StrHelper::split(str_args, "&", substrs);
            for(auto &str : substrs)
            {
                size_t pos = str.find("=");
                std::string key = str.substr(0, pos);
                std::string val = str.substr(pos+1);
                args[key] = val;
            }
        }

        std::string getArgs()
        {
            std::string result;
            for(auto &[a, b] : args)
            {
                result += a + "=" + b + "&";
            }
            return result;
        }
    };


    using QueueMap = std::unordered_map<std::string, Msgqueue::ptr>;

    class MsgQueueMapper
    {
        public:
            MsgQueueMapper(const std::string &dbfile):_sql_helper(dbfile)
            {
                std::string path = FileHelper::ParentDirectory(dbfile);
                FileHelper::createDirectory(path);
                assert(_sql_helper.open());
                createTable();    
            }

            void createTable()
            {
                std::stringstream sql;
                sql << "create table if not exists queue_table(";
                sql << "name varchar(32) primary key, ";
                sql << "durable int, ";
                sql << "exclusive int, ";
                sql << "auto_delete int, ";
                sql << "args varchar(128));";
                assert(_sql_helper.exec(sql.str(), nullptr, nullptr));
            }

            void removeTable()
            {
                std::string sql = "drop table if exists queue_table;";
                _sql_helper.exec(sql, nullptr, nullptr);
            }

            bool insert(Msgqueue::ptr &queue)
            {
                std::stringstream sql;
                sql << "insert into queue_table values( ";
                sql << "'" << queue->name << "', ";
                sql << queue-> durable << ",";
                sql << queue-> exclusive << ",";
                sql << queue-> auto_delete << ",";
                sql << "'" << queue->getArgs() << "');";
                return _sql_helper.exec(sql.str(), nullptr, nullptr);
            }

            void remove(const std::string &name)
            {
                std::stringstream sql;
                sql << "delete from queue_table where name= ";
                sql << "'" << name << "'";
                _sql_helper.exec(sql.str(), nullptr, nullptr);
            }

            QueueMap recovery()
            {
                QueueMap result;
                std::string sql = "select name, durable, exclusive, auto_delete, args from queue_table;";
                _sql_helper.exec(sql, selectCallback, &result);
                return result;
            }
        private:
            static int selectCallback(void *arg, int numcol, char **row, char **fields)
            {
                QueueMap *result = (QueueMap*)arg;
                Msgqueue::ptr mq = std::make_shared<Msgqueue>();
                mq->name = row[0];
                mq->durable = (bool)std::stoi(row[1]);
                mq->exclusive = (bool)std::stoi(row[2]);
                mq->auto_delete = (bool)std::stoi(row[3]);
                if(row[4])mq->setArgs(row[4]);
                result->insert(std::make_pair(mq->name, mq));
                return 0;
            }

        private:
            SqliteHelper _sql_helper;
    };


    class MsgQueueManager
    {
        public:
            using ptr = std::shared_ptr<MsgQueueManager>;
            MsgQueueManager(const std::string &dbfile):_mapper(dbfile)
            {
                _msg_queues = _mapper.recovery();
            }

            bool declareQueue(const std::string &qname,
            bool qdurable,
            bool qexclusive,
            bool qauto_delete,
            const google::protobuf::Map<std::string, std::string> &qargs){
                std::unique_lock<std::mutex> lock(_mutex);
                auto it = _msg_queues.find(qname);
                if(it != _msg_queues.end())
                {
                    return true;
                }
                Msgqueue::ptr mq = std::make_shared<Msgqueue>();
                mq->name = qname;
                mq->durable = qdurable;
                mq->exclusive = qexclusive;
                mq->auto_delete = qauto_delete;
                mq->args = qargs;
                if(qdurable == true)
                {
                    bool ret = _mapper.insert(mq);
                    if(ret == false)return false;
                }
                _msg_queues.insert(std::make_pair(qname, mq));
                return true;
            }

            void deleteQueue(const std::string &name)
            {
                std::unique_lock<std::mutex> lock (_mutex);
                auto it = _msg_queues.find(name);
                if(it == _msg_queues.end())
                {
                    return;
                }
                if(it->second->durable == true)_mapper.remove(name);
                _msg_queues.erase(name);
            }

            Msgqueue::ptr selectQueue(const std::string &name)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                auto it = _msg_queues.find(name);
                if(it == _msg_queues.end())
                {
                    return Msgqueue::ptr();
                }
                return it->second;
            }

            QueueMap allQueues()
            {
                std::unique_lock<std::mutex> lock(_mutex);
                return _msg_queues;
            }

            bool exists(const std::string &name)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                auto it = _msg_queues.find(name);
                if(it == _msg_queues.end())
                {
                    return false;
                }
                return true;
            }

            size_t size()
            {
                std::unique_lock<std::mutex> lock(_mutex);
                return _msg_queues.size();
            }

            void clear()
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _mapper.removeTable();
                _msg_queues.clear();
            }

        private:
            std::mutex _mutex;
            MsgQueueMapper _mapper;
            QueueMap _msg_queues;
    };

}
#endif










