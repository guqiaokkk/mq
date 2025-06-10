#include "../mqserver/mq_queue.hpp"
#include <gtest/gtest.h>

kjymq::MsgQueueManner::ptr mqmp;

class QueueTest: public testing::Environment
{ 
    public:
        virtual void SetUp() override
        {
            mqmp = std::make_shared<kjymq::MsgQueueManner>("./data/meta.db");
        }
        virtual void TearDown() override
        {
            mqmp->clear();
        }


};


// TEST(queue_test, insert_test)
// {
//     std::unordered_map<std::string, std::string> map = {{"k1", "v1"}};
//     mqmp->declareQueue("queue1", true, false, false, map);
//     mqmp->declareQueue("queue2", true, false, false, map);
//     mqmp->declareQueue("queue3", true, false, false, map);
//     mqmp->declareQueue("queue4", true, false, false, map);
//     ASSERT_EQ(mqmp->size(), 4);
// }//注释掉，模拟启动时从数据库恢复数据

TEST(queue_test, select_test)
{
    ASSERT_EQ(mqmp->exists("queue1"), false);
    ASSERT_EQ(mqmp->exists("queue2"), true);
    ASSERT_EQ(mqmp->exists("queue3"), false);
    ASSERT_EQ(mqmp->exists("queue4"), true);

    kjymq::Msgqueue::ptr mqp = mqmp->selectQueue("queue2");
    ASSERT_NE(mqp.get(), nullptr);
    ASSERT_EQ(mqp->name, "queue2");
    ASSERT_EQ(mqp->durable, true);
    ASSERT_EQ(mqp->exclusive, false);
    ASSERT_EQ(mqp->auto_delete, false);
    ASSERT_EQ(mqp->getArgs(), std::string("k1=v1&"));
}

TEST(queue_test, remove_test)
{
    mqmp->deleteQueue("queue2");
    ASSERT_EQ(mqmp->exists("queue2"), false);
}


int main(int argc, char *argv[])
{
    testing::InitGoogleTest(&argc, argv);
    testing::AddGlobalTestEnvironment(new QueueTest);
    RUN_ALL_TESTS();
    return 0;
}