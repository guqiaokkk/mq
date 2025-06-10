#include "../mqserver/mq_exchange.hpp"
#include <gtest/gtest.h>

kjymq::ExchangeManager::ptr emp;


class ExchangeTest:public testing::Environment{
    public:
        virtual void SetUp() override
        {
            emp = std::make_shared<kjymq::ExchangeManager> ("./data/meta.db");
        }
        virtual void TearDown() override
        {
           //emp->clear();
        }
};

// TEST(exchange_test, insert_test) {
//     std::unordered_map<std::string, std::string> map;
//     emp->declareExchange("exchange1", kjymq::ExchangeType::DIRECT, true, false, map);
//     emp->declareExchange("exchange2", kjymq::ExchangeType::DIRECT, true, false, map);
//     emp->declareExchange("exchange3", kjymq::ExchangeType::DIRECT, true, false, map);
//     emp->declareExchange("exchange4", kjymq::ExchangeType::DIRECT, true, false, map);
//     ASSERT_EQ(emp->size(), 4);
// }

// TEST(exchange_test, select_test) {
//     ASSERT_EQ(emp->exists("exchange1"), true);
//     ASSERT_EQ(emp->exists("exchange2"), true);
//     ASSERT_EQ(emp->exists("exchange4"), true);
//     kjymq::Exchange::ptr exp = emp->selectExchange("exchange2");
//     ASSERT_EQ(exp->durable, true);
//     ASSERT_EQ(exp->auto_delete, false);
//     ASSERT_EQ(exp->type, kjymq::ExchangeType::DIRECT);
// }删除前版本

TEST(exchange_test, select_test) {
    ASSERT_EQ(emp->exists("exchange1"), true);
    ASSERT_EQ(emp->exists("exchange2"), false);
    ASSERT_EQ(emp->exists("exchange3"), true);
    ASSERT_EQ(emp->exists("exchange4"), true);
    kjymq::Exchange::ptr exp = emp->selectExchange("exchange2");

}


// TEST(exchange_test, remove_test) {
//     emp->deleteExchange("exchange2");
//     kjymq::Exchange::ptr exp = emp->selectExchange("exchange2");
//     ASSERT_EQ(exp.get(), nullptr);
//     ASSERT_EQ(emp->exists("exchange2"), false);
// }


int main(int argc,char *argv[])
{
    testing::InitGoogleTest(&argc, argv);
    testing::AddGlobalTestEnvironment(new ExchangeTest);
    RUN_ALL_TESTS();
    return 0;
}