#include "../mqserver/mq_message.hpp"
#include <gtest/gtest.h>

kjymq::MessageManner::ptr mmp;

class MsgTest: public testing::Environment
{ 
    public:
        virtual void SetUp() override
        {
            mmp = std::make_shared<kjymq::MessageManner>("./data/message/");
            mmp->InitQueueMessage("queue1");
        }
        virtual void TearDown() override
        {
           //mmp->clear();
        }
};

//新增消息测试：新增消息，观察可获取消息数量，持久化消息数量
// TEST(msg_test, insert_test)
// {
//     kjymq::BasicProperties properties;
//     properties.set_id(kjymq::UUIDhelper::UUID());
//     properties.set_delivery_mode(kjymq::DeliveryMode::DURABLE);
//     properties.set_routing_key("news.music.pop");

//     mmp->insert("queue1", &properties, "hello_world1", true);
//     mmp->insert("queue1", nullptr, "hello_world2", true);
//     mmp->insert("queue1", nullptr, "hello_world3", true);
//     mmp->insert("queue1", nullptr, "hello_world4", true);
//     mmp->insert("queue1", nullptr, "hello_world5", false);

//     ASSERT_EQ(mmp->getable_count("queue1"), 5);
//     ASSERT_EQ(mmp->total_count("queue1"), 4);
//     ASSERT_EQ(mmp->waitack_count("queue1"),  0);
//     ASSERT_EQ(mmp->durable_count("queue1"), 4);


// }

//恢复历史数据测试
// TEST(message_test, recovery_test) {
//         mmp->InitQueueMessage("queue1");
//         ASSERT_EQ(mmp->getable_count("queue1"), 4);
//         kjymq::MessagePtr msg1 = mmp->Front("queue1");
//         ASSERT_NE(msg1.get(), nullptr);
//         ASSERT_EQ(msg1->payload().body(), std::string("hello_world1"));
//         ASSERT_EQ(mmp->getable_count("queue1"), 3);
//         ASSERT_EQ(mmp->waitack_count("queue1"), 1);
    
//         kjymq::MessagePtr msg2 = mmp->Front("queue1");
//         ASSERT_NE(msg2.get(), nullptr);
//         ASSERT_EQ(msg2->payload().body(), std::string("hello_world2"));
//         ASSERT_EQ(mmp->getable_count("queue1"), 2);
//         ASSERT_EQ(mmp->waitack_count("queue1"), 2);
    
//         kjymq::MessagePtr msg3 = mmp->Front("queue1");
//         ASSERT_NE(msg3.get(), nullptr);
//         ASSERT_EQ(msg3->payload().body(), std::string("hello_world3"));
//         ASSERT_EQ(mmp->getable_count("queue1"), 1);
//         ASSERT_EQ(mmp->waitack_count("queue1"), 3);
    
//         kjymq::MessagePtr msg4 = mmp->Front("queue1");
//         ASSERT_NE(msg4.get(), nullptr);
//         ASSERT_EQ(msg4->payload().body(), std::string("hello_world4"));
//         ASSERT_EQ(mmp->getable_count("queue1"), 0);
//         ASSERT_EQ(mmp->waitack_count("queue1"), 4);
//     }

//获取消息测试：获取一条消息，在不进行确认的情况下，查看可获取消息数量，待确认消息数量，以及测试消息获取的顺序
// TEST(message_test, select_test) {
//         ASSERT_EQ(mmp->getable_count("queue1"), 5);
//         kjymq::MessagePtr msg1 = mmp->Front("queue1");
//         ASSERT_NE(msg1.get(), nullptr);
//         ASSERT_EQ(msg1->payload().body(), std::string("hello_world1"));
//         ASSERT_EQ(mmp->getable_count("queue1"), 4);
//         ASSERT_EQ(mmp->waitack_count("queue1"), 1);
    
//         kjymq::MessagePtr msg2 = mmp->Front("queue1");
//         ASSERT_NE(msg2.get(), nullptr);
//         ASSERT_EQ(msg2->payload().body(), std::string("hello_world2"));
//         ASSERT_EQ(mmp->getable_count("queue1"), 3);
//         ASSERT_EQ(mmp->waitack_count("queue1"), 2);
    
//         kjymq::MessagePtr msg3 = mmp->Front("queue1");
//         ASSERT_NE(msg3.get(), nullptr);
//         ASSERT_EQ(msg3->payload().body(), std::string("hello_world3"));
//         ASSERT_EQ(mmp->getable_count("queue1"), 2);
//         ASSERT_EQ(mmp->waitack_count("queue1"), 3);
    
//         kjymq::MessagePtr msg4 = mmp->Front("queue1");
//         ASSERT_NE(msg4.get(), nullptr);
//         ASSERT_EQ(msg4->payload().body(), std::string("hello_world4"));
//         ASSERT_EQ(mmp->getable_count("queue1"), 1);
//         ASSERT_EQ(mmp->waitack_count("queue1"), 4);
    
//         kjymq::MessagePtr msg5 = mmp->Front("queue1");
//         ASSERT_NE(msg5.get(), nullptr);
//         ASSERT_EQ(msg5->payload().body(), std::string("hello_world5"));
//         ASSERT_EQ(mmp->getable_count("queue1"), 0);
//         ASSERT_EQ(mmp->waitack_count("queue1"), 5);
//     }
//删除消息测试：确认一条消息，查看持久化消息数量，待确认消息数量
// TEST(message_test, delete_test) {
//     ASSERT_EQ(mmp->getable_count("queue1"), 5);
//     kjymq::MessagePtr msg1 = mmp->Front("queue1");
//     ASSERT_NE(msg1.get(), nullptr);
//     ASSERT_EQ(msg1->payload().body(), std::string("hello_world1"));
//     ASSERT_EQ(mmp->getable_count("queue1"), 4);
//     ASSERT_EQ(mmp->waitack_count("queue1"), 1);
//     mmp->ack("queue1", msg1->payload().properties().id());
//     ASSERT_EQ(mmp->waitack_count("queue1"), 0);
//     ASSERT_EQ(mmp->durable_count("queue1"), 3);
//     ASSERT_EQ(mmp->total_count("queue1"), 4);
// }

TEST(message_test, one_test)
{
    ASSERT_EQ(mmp->getable_count("queue1"), 3);
    ASSERT_EQ(mmp->durable_count("queue1"), 3);
    ASSERT_EQ(mmp->total_count("queue1"), 3);

}



int main(int argc, char *argv[])
{
    testing::InitGoogleTest(&argc, argv);
    testing::AddGlobalTestEnvironment(new MsgTest);
    RUN_ALL_TESTS();
    return 0;
}