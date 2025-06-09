#ifndef __M_BINDING_H__
#define __M_BINDING_H__
#include "../mqcommon/mq_logger.hpp"
#include "../mqcommon/mq_helper.hpp"
#include "../mqcommon/mq_msg.pb.h"
#include <iostream>
#include <unordered_map>
#include <mutex>
#include <memory>



namespace kjymq{
    struct Binding
    {
        using ptr = std::shared_ptr<Binding>;
    
        std::string exchange_name;
        std::string msgqueue_name;
        std::string binding_key;

        Binding(){}
        Binding(const std::string &ename, const std::string &qname, const std::string &key):
        exchange_name(ename),
        msgqueue_name(qname),
        binding_key(key){}


    };


    //队列与绑定信息是一一对应的（因为是给某个交换机去绑定队列，因此一个交换机可能会有多个队列的绑定信息）
    //因此先定义一个队列名，与绑定信息的映射关系，这个是为了方便通过队列名查找绑定信息
    using MsgQueueBindMap = std::unordered_map<std::string, Binding::ptr>;

    //然后定义一个交换机名称与队列绑定信息的映射关系，这个map中包含了所有的绑定信息，并且以交换机为单元进行了区分
    using BindingMap = std::unordered_map<std::string, MsgQueueBindMap>;
    //这里相当于一个交换机名称对应一个map,map里每个队列有自己的binding，但map中队列名不能重复
    //也就是一个交换机对一个队列只能binding一次，不能以不同的信息绑定一个队列多次

    //std::unordered_map<std::string, Binding::ptr>;  队列与绑定  ，
    //std::unordered_map<std::string, Binding::ptr>;  交换机与绑定
    //采用上边两个结构，则删除交换机相关绑定信息的时候，不仅要删除交换机映射，还要删除对应队列中的映射，否则对象得不到释放，所以这里我们不使用


    class BindingMapper
    {
        public:
            BindingMapper(const std::string &dbfile):_sql_helper(dbfile){
                std::string path = FileHelper::ParentDirectory(dbfile);
                FileHelper::createDirectory(path);
                assert(_sql_helper.open());
                createTable();
            }

            void createTable()
            {
                std::stringstream sql;
                sql << "create table if not exists binding_table(";
                sql << "exchange_name varchar(32), ";
                sql << "msgqueue_name varchar(32), ";
                sql << "binding_key varchar(128)); ";
                assert(_sql_helper.exec(sql.str(), nullptr, nullptr));
            }

            void removeTable()
            {
                std::string sql = "drop table if exists binding_table; ";
                _sql_helper.exec(sql, nullptr, nullptr);
            }

            bool insert(Binding::ptr &binding)
            {
                std::stringstream sql;
                sql << "insert into binding_table values( ";
                sql << "'" << binding->exchange_name << "', ";
                sql << "'" << binding->msgqueue_name << "', ";
                sql << "'" << binding->binding_key << "'); ";
                return _sql_helper.exec(sql.str(), nullptr, nullptr);
            }

            void remove(const std::string &ename, const std::string &qname)
            {
                std::stringstream sql;
                sql << "delete from binding_table where ";
                sql << "exchange_name = '" << ename << "' and ";
                sql << "msgqueue_name = '" << qname << "';";
                _sql_helper.exec(sql.str(), nullptr, nullptr); 
            }

            void removeExchangeBindings(const std::string &ename)
            {
                std::stringstream sql;
                sql << "delete from binding_table where ";
                sql << "exchange_name = '" << ename << "'";
                _sql_helper.exec(sql.str(), nullptr, nullptr);
            }

            void removeMsgQueueBindings(const std::string &qname) {
                std::stringstream sql;
                sql << "delete from binding_table where ";
                sql << "msgqueue_name='" << qname << "';";
                _sql_helper.exec(sql.str(), nullptr, nullptr);
            }

            BindingMap recovery()
            {
                BindingMap result;
                std::string sql = "select exchange_name, msgqueue_name, binding_key from binding_table;";
                _sql_helper.exec(sql, selectCallback, &result);
                return result;
            }

        private:
            static int selectCallback(void *arg, int numcol, char **row, char **fields)
            {
                BindingMap* result = (BindingMap *)arg;
                Binding::ptr bp = std::make_shared<Binding>(row[0], row[1], row[2]);
                
                //为了防止 交换机相关的绑定信息已经存在，因此不能直接创建队列映射，进行添加，这样会覆盖历史数据
                //因此得先获取交换机对应的映射对象，往里边添加数据
                //但是，若这时候没有交换机对应的映射信息，因此这里的获取要使用引用（会保证不存在则自动创建）
                MsgQueueBindMap &mqb = (*result)[bp->exchange_name];
                mqb.insert(std::make_pair(bp->msgqueue_name, bp));
                return 0;
            }

        private:
            SqliteHelper _sql_helper;
    };


    class BindingManager
    {
        public:
            using ptr = std::shared_ptr<BindingManager>;
            BindingManager(const std::string &dbfile):_mapper(dbfile)
            {
                _bindings = _mapper.recovery();
            }

            bool bind(const std::string &ename, const std::string &qname, const std::string &key, bool durable)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                auto it = _bindings.find(ename);
                if(it != _bindings.end() && it->second.find(qname) != it->second.end())
                {
                    return true;
                }
                //绑定信息是否需要持久化，取决于交换机数据是持久化的，以及队列数据也是持久化的。这里我们为了简化通过外界传入durable来判断
                Binding::ptr bp = std::make_shared<Binding>(ename, qname, key);
                if(durable){
                    bool ret = _mapper.insert(bp);
                    if(!ret) return false;
                }
                auto &qp = _bindings[ename];
                qp.insert(std::make_pair(qname, bp));
                return true;
            }

            void unBind(const std::string &ename, const std::string &qname)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                auto eit = _bindings.find(ename);
                if(eit == _bindings.end())
                {
                    return;//没有交换机相关的绑定信息
                }
                
                auto qit = eit->second.find(qname);
                if(qit == eit->second.end())
                {
                    return;//交换机中没有qname队列相关的绑定信息
                }

                _mapper.remove(ename, qname);
                _bindings[ename].erase(qname);
            }

            void removeExchangeBindings(const std::string &ename)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _mapper.removeExchangeBindings(ename);
                _bindings.erase(ename);
            }

            void removeMsgQueueBindings(const std::string &qname)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _mapper.removeMsgQueueBindings(qname);
                for(auto it = _bindings.begin(); it != _bindings.end(); ++it)
                {
                    //遍历每个交换机的绑定信息，从中移除指定队列的相关信息
                    it->second.erase(qname);
                }
            }

            MsgQueueBindMap getExchangeBindings(const std::string &ename)//返回与指定交换机名称相关的所有队列绑定信息。
            {
                std::unique_lock<std::mutex> lock(_mutex);
                auto eit = _bindings.find(ename);
                if(eit == _bindings.end())
                {
                    return MsgQueueBindMap();
                }
                return eit->second;//交换机是唯一的,不会重名
            }

            Binding::ptr getBinding(const std::string &ename, const std::string &qname)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                auto eit = _bindings.find(ename);
                if(eit == _bindings.end())
                {
                    return Binding::ptr();
                }
                auto qit = eit->second.find(qname);
                if(qit == eit->second.end())
                {
                    return Binding::ptr();
                }
                return qit->second;
            }

            bool exists(const std::string &ename, const std::string &qname)
            {
                std::unique_lock<std::mutex> lock(_mutex);
                auto eit = _bindings.find(ename);
                if(eit == _bindings.end())return false;
                auto qit = eit->second.find(qname);
                if(qit == eit->second.end())return false;
                return true;
            }

            size_t size()
            {
                size_t ans = 0;
                std::unique_lock<std::mutex> lock(_mutex);
                for(auto it = _bindings.begin(); it != _bindings.end(); ++it)
                {
                    //遍历每个交换机的绑定信息，从中获取每个交换机中绑定    队列的数量
                    ans += it->second.size();
                }
                return ans;
            }

            void clear()
            {
                std::unique_lock<std::mutex> lock(_mutex);
                _mapper.removeTable();
                _bindings.clear();
            }


        private:
            std::mutex _mutex;
            BindingMapper _mapper;
            BindingMap _bindings;
    };

}


#endif
























