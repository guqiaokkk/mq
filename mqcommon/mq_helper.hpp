#ifndef __M_HELPER_H__
#define __M_HELPER_H__
#include <iostream>
#include <vector>
#include <string>
#include <sqlite3.h>
#include <random>
#include <sstream>
#include <iomanip>
#include <atomic>
#include <sys/stat.h>
#include <fstream>
#include <cstring>
#include <cerrno>

#include "mq_logger.hpp"

namespace kjymq
{

    class SqliteHelper
    {
    public:
        typedef int (*SqliteCallback)(void *, int, char **, char **);
        SqliteHelper(const std::string &dbfile) : _dbfile(dbfile), _handler(nullptr) {}

        bool open(int save_level = SQLITE_OPEN_FULLMUTEX)
        {
            // int sqlite3_open_v2(const char *filename, sqlite3 **ppDb, int flags, const char *zVfs );
            int ret = sqlite3_open_v2(_dbfile.c_str(), &_handler, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | save_level, nullptr);
            if (ret != SQLITE_OK)
            {
                ELOG("创建/打开sqlite数据库失败: %s", sqlite3_errmsg(_handler));
                return false;
            }
            return true;
        }

        bool exec(const std::string &sql, SqliteCallback cb, void *args)
        {
            // int sqlite3_exec(sqlite3*, char *sql, int (*callback)(void*,int,char**,char**), void* arg, char **err)
            int ret = sqlite3_exec(_handler, sql.c_str(), cb, args, nullptr);
            if (ret != SQLITE_OK)
            {
                ELOG("%s \n执行语句失败: %s", sql.c_str() ,sqlite3_errmsg(_handler));
                return false;
            }
            return true;
        }

        void close()
        {
            // int sqlite3_close_v2(sqlite3*);
            if (_handler)
                sqlite3_close_v2(_handler);
        }

    private:
        std::string _dbfile;
        sqlite3 *_handler;
    };

    class StrHelper
    {
    public:
        static size_t split(const std::string &tmp, const std::string &sep, std::vector<std::string> &arr)
        {
            size_t pos, idx = 0;
            while (idx < tmp.size())
            {
                pos = tmp.find(sep, idx);
                if (pos == std::string::npos)
                {
                    arr.push_back(tmp.substr(idx));
                    return arr.size();
                }
                if (pos == idx)
                {
                    idx = pos + sep.size();
                    continue;
                }

                arr.push_back(tmp.substr(idx, pos - idx));
                idx = pos + sep.size();
            }
            return arr.size();
        }
    };

    class UUIDhelper
    {
        public:
            static std::string UUID()
            {
                std::random_device rd;
                std::mt19937_64 gernator(rd());
                std::uniform_int_distribution<int> distribution(0, 255); //[0,255]
                std::stringstream ss;
                for (int i = 0; i < 8; i++)
                {
                    ss << std::setw(2) << std::setfill('0') << std::hex << distribution(gernator);
                    if (i == 3 || i == 5 || i == 7)
                    {
                        ss << "-";
                    }
                }

                static std::atomic<size_t> seq(1);
                size_t num = seq.fetch_add(1);
                for (int i = 7; i >= 0; i--)
                {
                    ss << std::setw(2) << std::setfill('0') << std::hex << ((num >> (i * 8)) & 0xff);
                    if (i == 6)
                        ss << "-";
                }
                return ss.str();
            }
    };

    class FileHelper
    {
    public:
        FileHelper(std::string filename) : _filename(filename) {}

        bool exist()
        {
            struct stat st;
            return (stat(_filename.c_str(), &st) == 0);
        }

        size_t size()
        {
            struct stat st;
            int ret = stat(_filename.c_str(), &st);
            if (ret < 0)
            {
                return 0;
            }
            return st.st_size;
        }

        bool read(char *body, size_t offset, size_t len)
        {
            // 1.打开文件
            std::ifstream ifs(_filename, std::ios::binary | std::ios::in); // 以二进制和只读方式打开
            if (ifs.is_open() == false)
            {
                ELOG("%s 打开失败！", _filename.c_str());
                return false;
            }
            // 2.跳转文件读写位置
            ifs.seekg(offset, std::ios::beg);
            // 3.读取文件数据
            ifs.read(body, len);
            if (ifs.good() == false)
            {
                ELOG("%s 文件数据读取失败", _filename.c_str());
                ifs.close();
                return false;
            }
            // 4.关闭文件
            ifs.close();
            return true;
        }
        bool read(std::string &body)
        {
            // 获取文件大小，根据文件大小给body定制空间
            size_t fsize = this->size();
            body.resize(fsize);
            return read(&body[0], 0, fsize);

        }

        bool write(const char *body, size_t offset, size_t len)
        {
            // 1.打开文件
            std::fstream fs(_filename, std::ios::binary | std::ios::in | std::ios::out);
            if (fs.is_open() == false)
            {
                ELOG("%s 打开失败！", _filename.c_str());
                return false;
            }
            // 2.跳转到写入位置
            fs.seekp(offset, std::ios::beg);
            // 3.写入文件数据
            fs.write(body, len);
            if (fs.good() == false)
            {
                ELOG("%s 文件数据写入失败", _filename.c_str());
                return false;
            }
            // 4.关闭文件
            fs.close();
            return true;
        }
        bool write(const std::string body)
        {
            return (write(body.c_str(), 0, body.size()));
        }

        bool rename(const std::string &newname)
        {
            return (::rename(_filename.c_str(), newname.c_str()) == 0);
        }

        static std::string ParentDirectory(const std::string &filename)
        {
             // /aaa/bb/ccc/ddd/test.txt
             size_t pos = filename.find_last_of("/");
             if(pos == std::string::npos)
             {
                //说明路径是一个相对路径，且没有父目录，就在当前目录  eg:test.txt
                return "./";
             }
             std::string path = filename.substr(0, pos);
             return path;
        }

        static bool createFile(const std::string &filename)
        {
            std::fstream cfs(filename, std::ios::binary | std::ios::out);
            if(cfs.is_open() == false)
            {
                ELOG("%s 创建文件失败", filename.c_str());
                return false;
            }
            cfs.close();
            return true;
        }

        static bool removeFile(const std::string &filename)
        {
            return (::remove(filename.c_str()) == 0);
        }

        static bool createDirectory(const std::string &path)//递归地创建一个多级目录路径
        {
            //  aaa/bbb/ccc    cccc
            // 在多级路径创建中，我们需要从第一个父级目录开始创建
            size_t pos, idx = 0;
            while (idx < path.size())
            {
                pos = path.find("/", idx);
                if(pos == std::string::npos)
                {
                    return (mkdir(path.c_str(), 0775)==0);//如果路径中没有更多的 /，直接尝试创建整个路径。
                }
                std::string subpath = path.substr(0, pos);
                int ret = mkdir(subpath.c_str(), 0775);
                if(ret != 0 && errno != EEXIST)
                {
                    ELOG("%s 创建目录失败: %s", subpath.c_str(), strerror(errno));
                    return false;
                }
                idx = pos + 1;
            }            
            return true;
        }

        static bool removeDirectory(const std::string &path)
        {
            std::string cmd = "rm -rf " + path;
            return (system(cmd.c_str()) != -1);
        }

    private:
        std::string _filename;
    };

}

#endif