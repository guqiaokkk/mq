#include "../mqcommon/helper.hpp"

int main()
{
    // kjymq::FileHelper helper("../mqcommon/logger.hpp");
    // DLOG("文件是否存在:%d", helper.exist());
    // DLOG("文件的大小: %ld", helper.size());

    // kjymq::FileHelper tmp_helper("./aaa/bbb/ccc/tmp.hpp");
    // if(tmp_helper.exist() == false){
    //     std::string path  = kjymq::FileHelper::ParentDirectory("./aaa/bbb/ccc/tmp.hpp");
    //     if(kjymq::FileHelper(path).exist() == false){
    //         kjymq::FileHelper::createDirectory(path);
    //     }
    //     kjymq::FileHelper::createFile("./aaa/bbb/ccc/tmp.hpp");
    // }
    // std::string body;
    // helper.read(body);
    // tmp_helper.write(body);
    
    //kjymq::FileHelper tmp_helper("./aaa/bbb/ccc/tmp.hpp");

    // char str[16] = {0};
    // tmp_helper.read(str, 8,9);
    // DLOG("[%s]", str);
    
    // tmp_helper.write("12345678901", 8, 11);

    //tmp_helper.rename("./aaa/bbb/ccc/test.hpp");

    //kjymq::FileHelper::removeFile("./aaa/bbb/ccc/test.hpp");

    // kjymq::FileHelper::removeDirectory("./aaa");

    return 0;
}