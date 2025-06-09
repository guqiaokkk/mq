#ifndef __M_ROUTE_H__
#define __M_ROUTE_H__

#include <iostream>
#include "../mqcommon/mq_logger.hpp"
#include "../mqcommon/mq_helper.hpp"
#include "../mqcommon/mq_msg.pb.h"


namespace kjymq{
    class Router
    {
        public:
            //判断routingkey是否合法
            static bool isLegalRoutingKey(const std::string &rtk)
            {
                 //routing_key：只需要判断是否包含有非法字符即可， 合法字符( a~z, A~Z, 0~9, ., _)
                for(auto &s : rtk)
                {
                    if((s >= 'a' && s <= 'z') ||
                       (s >= 'A' && s <= 'Z') ||
                       (s >= '0' && s <= '9') ||
                       (s == '_' || s == '.')  )
                       {
                        continue;
                       }
                    return false;
                }
                return true;
            }

            //判断bindingkey是否合法
            static bool isLegalBindingKey(const std::string &bdk)
            {
                //1. 判断是否包含有非法字符， 合法字符：a~z, A~Z, 0~9, ., _, *, #
                for(auto &s : bdk)
                {
                    if((s >= 'a' && s <= 'z') ||
                       (s >= 'A' && s <= 'Z') ||
                       (s >= '0' && s <= '9') ||
                       (s == '_' || s == '.') ||
                       (s == '#' || s == '*')  ) 
                       {
                        continue;
                       }
                    return false;
                }
                //2. *和#必须独立存在，如:  news.music#.*.#
                //如果某个单词：
                //长度 超过 1 个字符（如 ne#ws）。
                //且包含通配符 # 或 *（如 mus*ic 或 ne#ws）。
                //则判定为非法格式，返回 false
                std::vector<std::string> words;
                StrHelper::split(bdk, ".", words);
                for(auto &s : words)
                {
                    if(s.size() > 1 &&
                       (s.find("#") != std::string::npos ||
                        s.find("*") != std::string::npos) )
                        {
                            return false;
                        }
                }
                //3. *和#不能连续出现
                for(int i = 1; i < words.size(); i++)
                {
                    if(words[i] == "#" && words[i-1] == "#")
                    {
                        return false;
                    }
                    if(words[i] == "#" && words[i-1] == "*")
                    {
                        return false;
                    }
                    if(words[i] == "*" && words[i-1] == "#")
                    {
                        return false;
                    }
                }

                return true;
            }

            //根据交换机类型，实现消息队列中的路由匹配逻辑
            static bool route(ExchangeType type, const std::string &rtk, const std::string &bdk)
            {
                if(type == ExchangeType::FANOUT)
                {
                    return true;
                }
                else if (type == ExchangeType::DIRECT)
                {
                    return (rtk == bdk);
                }
                
                //主题交换：要进行模式匹配    news.#   &   news.music.pop
                //1. 将binding_key与routing_key进行字符串分割，得到各个的单词数组
                std::vector<std::string> rkeys, bkeys;
                int n_rkey = StrHelper::split(rtk, ".", rkeys);
                int n_bkey = StrHelper::split(bdk, ".", bkeys);

                //2. 定义标记数组，并初始化[0][0]位置为true，其他位置为false
                std::vector<std::vector<bool>> dp(n_bkey + 1, std::vector<bool>(n_rkey + 1, false));
                dp[0][0] = true;
                
                //3. 如果binding_key以#起始，则将#对应行的第0列置为1
                for(int i = 1; i <= n_bkey; i++)
                {
                    if(bkeys[i-1] == "#")
                    {
                        dp[i][0] = true;
                        continue;
                    }
                    break;
                }

                //4. 使用routing_key中的每个单词与binding_key中的每个单词进行匹配并标记数组
                for(int i = 1; i <= n_bkey; i++)
                {
                    for(int j = 1; j <= n_rkey; j++)
                    {
                        //如果当前bkey是个*，或者两个单词相同，表示单词匹配成功，则从左上方继承结果
                        if(bkeys[i-1] == rkeys[j-1] || bkeys[i-1] == "*")
                        {
                            dp[i][j] = dp[i-1][j-1];
                        }
                        else if(bkeys[i-1] == "#")
                        {
                            dp[i][j] = dp[i-1][j] || dp[i-1][j-1] || dp[i][j-1];
                        }
                        //else{}初始时就已经给了false
                    }
                }
                return dp[n_bkey][n_rkey];
            }
    };
}

#endif