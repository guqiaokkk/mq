#ifndef __M_EXCHANGE_H__
#define __M_EXCHANGE_H__


#include "../mqcommon/mq_helper.hpp"
#include "../mqcommon/mq_msg.pb.h"
#include "../mqcommon/mq_logger.hpp"
#include <string>
#include <iostream>
#include <unordered_map>
#include <mutex>
#include <memory>

namespace kjymq{

struct Exchange{
    using ptr = std::shared_ptr<Exchange>;

    //1. 交换机名称
    std::string name;
     //2. 交换机类型
    ExchangeType type;
    //3. 交换机持久化标志
    bool durable;
    //4. 是否自动删除标志
    bool auto_delete;
    //5. 其他参数
    google::protobuf::Map<std::string, std::string> args;

    Exchange(){}

    Exchange(const std::string &ename, 
        ExchangeType etype,
        bool edurable,
        bool eauto_delete,
        const google::protobuf::Map<std::string, std::string> &eargs):name(ename),
        type(etype), durable(edurable),auto_delete(eauto_delete), args(eargs)
        {}

     //args存储键值对，在存储数据库的时候，会组织一个格式字符串进行存储 key=val&key=val....
     //内部解析str_args字符串，将内容存储到成员中
    void setArgs(const std::string &str_args)
    {
        //key=val&key=val&
        std::vector<std::string> substrs;
        StrHelper::split(str_args, "&", substrs);
        for(auto &str : substrs)
        {
            size_t pos = str.find("=");
            std::string key = str.substr(0,pos);
            std::string value = str.substr(pos+1);
            args[key] = value;
        }
    }

    //将args中的内容进行序列化后，返回一个字符串
    std::string getArgs()
    {
        std::string result;
        for(auto start = args.begin(); start != args.end(); ++start)
        {
            result += start->first + "=" + start->second + "&";
        }
        return result;
    }
};

using ExchangeMap = std::unordered_map<std::string, Exchange::ptr>;
    
//2. 定义交换机数据持久化管理类--数据存储在sqlite数据库中
class ExchangeMapper
{
    public:
        ExchangeMapper(const std::string &dbfile):_sql_helper(dbfile)
        {
            std::string path = FileHelper::ParentDirectory(dbfile);
            FileHelper::createDirectory(path);
            //如果目录已经存在，不会执行任何操作。
            //如果目录不存在，会递归创建所有缺失的目录。

            assert(_sql_helper.open());
            createTable();
        }

        void createTable()
        {
            #define CREATE_TABLE "create table if not exists exchange_table(\
            name varchar(32) primary key,\
            type int,\
            durable int,\
            auto_delete int,\
            args varchar(128));"
            bool ret = _sql_helper.exec(CREATE_TABLE, nullptr, nullptr);
            if(ret == false)
            {
                ELOG("创建交换机数据库表失败!");
                abort();//直接退出异常程序
            }
        }

        void removeTable()
        {
            #define DROP_TABLE "drop table if exists exchange_table;"
            bool ret = _sql_helper.exec(DROP_TABLE, nullptr, nullptr);
            if(ret == false)
            {
                ELOG("删除交换机数据表失败！");
                abort();//直接退出异常程序
            }
        }


        bool insert(Exchange::ptr &exp)
        {
            std::stringstream ss;
            ss << "insert into exchange_table values(";
            ss << "'" << exp->name << "',";
            ss << exp->type << ",";
            ss << exp->durable << ",";
            ss << exp->auto_delete << ",";
            ss << "'" << exp->getArgs() << "');";
            return _sql_helper.exec(ss.str(), nullptr, nullptr);
        }

        void remove(const std::string &name)
        {
            std::stringstream ss;
            ss << "delete from exchange_table where name= ";
            ss << "'" << name << "'";
            _sql_helper.exec(ss.str(), nullptr, nullptr);
        }

        //从数据库中恢复所有交换机的信息，并将这些信息加载到内存中的一个映射（ExchangeMap）中
        ExchangeMap recovery()
        {
            ExchangeMap result;
            std::string sql = "select name, type, durable, auto_delete, args from exchange_table";
            _sql_helper.exec(sql, selectCallback, &result);
            return result;
        }

    private:
        //将每一行查询结果转换为一个 Exchange 对象，并将其插入到 ExchangeMap 中    一个回调函数
        static int selectCallback(void *arg, int numcol, char **row, char **fields)
        {
            ExchangeMap *result = (ExchangeMap *)arg;
            auto exp = std::make_shared<Exchange>();
            //将第二列的值（交换机的类型）转换为整数，并强制转换为 bitmq::ExchangeType 枚举类型。
            exp->name = row[0];
            exp->type = (kjymq::ExchangeType)std::stoi(row[1]);
            exp->durable = (bool)std::stoi(row[2]);
            exp->auto_delete = (bool)std::stoi(row[3]);
            if(row[4])exp->setArgs(row[4]);
            result->insert(std::make_pair(exp->name, exp));
            return 0;//回调函数返回 0，表示成功处理了这一行数据。
        }


    private:
        SqliteHelper _sql_helper;
};

//3. 定义交换机数据内存管理类 
class ExchangeManager
{
    public:
        using ptr = std::shared_ptr<ExchangeManager>;
        ExchangeManager(const std::string &dbfile):_mapper(dbfile){
            _exchanges = _mapper.recovery();
        }
        //声明交换机
        bool declareExchange(const std::string &name,
        ExchangeType type, bool durable, bool auto_delete,
        const google::protobuf::Map<std::string, std::string> &args){
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _exchanges.find(name);
            if(it != _exchanges.end())
            {
                //交换机已经存在，不需要新增
                return true;
            }
            auto exp = std::make_shared<Exchange>(name, type, durable, auto_delete, args);
            if(durable == true)//如果交换机是持久化的（durable == true），则将其信息插入到数据库中
            {
                bool ret = _mapper.insert(exp);
                if(ret == false) return false;
            }
            //创建一个新的 Exchange 对象，并将其插入到 _exchanges 映射中。
            _exchanges.insert(std::make_pair(name, exp));
            return true;
        }
    
        //删除交换机
        void deleteExchange(const std::string &name)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _exchanges.find(name);
            if(it == _exchanges.end())
            {
                return;
            }
            if(it->second->durable == true)_mapper.remove(name);
            _exchanges.erase(name);  
        }

        //获取指定交换机对象
        Exchange::ptr selectExchange(const std::string &name)
        {
            std::unique_lock<std::mutex> lock (_mutex);
            auto it = _exchanges.find(name);
            if(it == _exchanges.end())
            {
                return Exchange::ptr();
                //Exchange::ptr 是一个 std::shared_ptr<Exchange> 的别名,默认构造的 std::shared_ptr 是空的，即它不指向任何对象。
            }
            return it->second;
        }

        //判断交换机是否存在(用上面的获取指定交换机对象也可以表示)
        bool exists(const std::string &name)
        {
            std::unique_lock<std::mutex> lock(_mutex);
            auto it = _exchanges.find(name);
            if(it == _exchanges.end())
            {
                return false;
            }
            return true;
        }

        size_t size()
        {
            std::unique_lock<std::mutex> lock(_mutex);
            return _exchanges.size();
        }

        void clear()
        {
            std::unique_lock<std::mutex> lock (_mutex);
            _mapper.removeTable();//删除数据库中的表
            _exchanges.clear();//清空内存映射表
        }

    private:
        std::mutex _mutex;
        ExchangeMapper _mapper;
        ExchangeMap _exchanges;
};

}

#endif