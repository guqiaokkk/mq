#include <iostream>
#include <string>
#include <vector>

std::string aaa = ".sda..dad.dad";

size_t split(const std::string &tmp, const std::string &sep, std::vector<std::string> &arr)
{
    // news....music.#.pop
    // 分割的思想：
    //  1. 从0位置开始查找指定字符的位置， 找到之后进行分割
    //  2. 从上次查找的位置继续向后查找指定字符
    size_t pos, idx = 0;
    while (idx < tmp.size())
    {
        pos = tmp.find(sep, idx);
        if(pos == std::string::npos)
        {   
            //没有找到,则从查找位置截取到末尾
            arr.push_back(tmp.substr(idx));
            return arr.size();
        }
        //pos == idx 代表两个分隔符之间没有数据，或者说查找起始位置就是分隔符
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

int main()
{
    std::string str = "...news....music.#.pop...";
    std::vector<std::string> arry;
    int n = split(str, ".", arry);
    for (auto &s : arry) {
        std::cout << s << std::endl;
    }
    return 0;
}