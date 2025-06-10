#include "../mqserver/mq_channel.hpp"

int main()
{
    kjymq::ChannelManager::ptr cmp = std::make_shared<kjymq::ChannelManager>();

    cmp->openChannel("kkkk",
                     muduo::net::TcpConnectionPtr(),
                     kjymq::ProtobufCodecPtr(),
                     std::make_shared<kjymq::ConsumerManager>(),
                     std::make_shared<kjymq::VirtualHost>("host1",  "./data/host1/message/", "./data/host1/host1.db"),
                     threadpool::ptr());
    return 0;
}