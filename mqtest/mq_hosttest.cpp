#include <gtest/gtest.h>
#include "../mqserver/mq_host.hpp"


class HostTest : public testing::Test {
    public:
        void SetUp() override {
            std::unordered_map<std::string, std::string> empty_map = std::unordered_map<std::string, std::string>();
            _host = std::make_shared<kjymq::VirtualHost>("host1", "./data/host1/message/", "./data/host1/host1.db");
            _host->declareExchange("exchange1", kjymq::ExchangeType::DIRECT, true, false, empty_map);
            _host->declareExchange("exchange2", kjymq::ExchangeType::DIRECT, true, false, empty_map);
            _host->declareExchange("exchange3", kjymq::ExchangeType::DIRECT, true, false, empty_map);

            _host->declareQueue("queue1", true, false, false, empty_map);
            _host->declareQueue("queue2", true, false, false, empty_map);
            _host->declareQueue("queue3", true, false, false, empty_map);

            _host->bind("exchange1", "queue1", "news.music.#");
            _host->bind("exchange1", "queue2", "news.music.#");
            _host->bind("exchange1", "queue3", "news.music.#");
            
            _host->bind("exchange2", "queue1", "news.music.#");
            _host->bind("exchange2", "queue2", "news.music.#");
            _host->bind("exchange2", "queue3", "news.music.#");

            _host->bind("exchange3", "queue1", "news.music.#");
            _host->bind("exchange3", "queue2", "news.music.#");
            _host->bind("exchange3", "queue3", "news.music.#");

            _host->basicPublish("queue1", nullptr, "Hello World-1");
            _host->basicPublish("queue1", nullptr, "Hello World-2");
            _host->basicPublish("queue1", nullptr, "Hello World-3");
            
            _host->basicPublish("queue2", nullptr, "Hello World-1");
            _host->basicPublish("queue2", nullptr, "Hello World-2");
            _host->basicPublish("queue2", nullptr, "Hello World-3");
            
            _host->basicPublish("queue3", nullptr, "Hello World-1");
            _host->basicPublish("queue3", nullptr, "Hello World-2");
            _host->basicPublish("queue3", nullptr, "Hello World-3");
        }
        void TearDown() override {
            _host->clear();
        }
    public:
        kjymq::VirtualHost::ptr _host;
};


TEST_F(HostTest, init_test) {
    ASSERT_EQ(_host->existsExchange("exchange1"), true);
    ASSERT_EQ(_host->existsExchange("exchange2"), true);
    ASSERT_EQ(_host->existsExchange("exchange3"), true);
    
    ASSERT_EQ(_host->existsQueue("queue1"), true);
    ASSERT_EQ(_host->existsQueue("queue2"), true);
    ASSERT_EQ(_host->existsQueue("queue3"), true);
    
    ASSERT_EQ(_host->existsBinding("exchange1", "queue1"), true);
    ASSERT_EQ(_host->existsBinding("exchange1", "queue2"), true);
    ASSERT_EQ(_host->existsBinding("exchange1", "queue3"), true);
    
    ASSERT_EQ(_host->existsBinding("exchange2", "queue1"), true);
    ASSERT_EQ(_host->existsBinding("exchange2", "queue2"), true);
    ASSERT_EQ(_host->existsBinding("exchange2", "queue3"), true);

    ASSERT_EQ(_host->existsBinding("exchange3", "queue1"), true);
    ASSERT_EQ(_host->existsBinding("exchange3", "queue2"), true);
    ASSERT_EQ(_host->existsBinding("exchange3", "queue3"), true);

    kjymq::MessagePtr msg1 = _host->basicConsume("queue1");
    ASSERT_EQ(msg1->payload().body(), std::string("Hello World-1"));
    kjymq::MessagePtr msg2 = _host->basicConsume("queue1");
    ASSERT_EQ(msg2->payload().body(), std::string("Hello World-2"));
    kjymq::MessagePtr msg3 = _host->basicConsume("queue1");
    ASSERT_EQ(msg3->payload().body(), std::string("Hello World-3"));
    kjymq::MessagePtr msg4 = _host->basicConsume("queue1");
    ASSERT_EQ(msg4.get(), nullptr);
}

TEST_F(HostTest, remove_exchange) {
    _host->deleteExchange("exchange1");
    ASSERT_EQ(_host->existsBinding("exchange1", "queue1"), false);
    ASSERT_EQ(_host->existsBinding("exchange1", "queue2"), false);
    ASSERT_EQ(_host->existsBinding("exchange1", "queue3"), false);
}

TEST_F(HostTest, remove_queue) {
    _host->deleteQueue("queue1");
    ASSERT_EQ(_host->existsBinding("exchange1", "queue1"), false);
    ASSERT_EQ(_host->existsBinding("exchange2", "queue1"), false);
    ASSERT_EQ(_host->existsBinding("exchange3", "queue1"), false);
    
    kjymq::MessagePtr msg1 = _host->basicConsume("queue1");
    ASSERT_EQ(msg1.get(), nullptr);
}

TEST_F(HostTest, ack_message) {
    kjymq::MessagePtr msg1 = _host->basicConsume("queue1");
    ASSERT_EQ(msg1->payload().body(), std::string("Hello World-1"));
    _host->basicack(std::string("queue1"), msg1->payload().properties().id());

    kjymq::MessagePtr msg2 = _host->basicConsume("queue1");
    ASSERT_EQ(msg2->payload().body(), std::string("Hello World-2"));
    _host->basicack(std::string("queue1"), msg2->payload().properties().id());

    kjymq::MessagePtr msg3 = _host->basicConsume("queue1");
    ASSERT_EQ(msg3->payload().body(), std::string("Hello World-3"));
    _host->basicack(std::string("queue1"), msg3->payload().properties().id());

}


int main(int argc, char *argv[])
{
    testing::InitGoogleTest(&argc, argv);
    RUN_ALL_TESTS();
    return 0;
}