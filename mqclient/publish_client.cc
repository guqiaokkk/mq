#include "mq_connection.hpp"


int main()
{
    //1. 实例化异步工作线程对象
    kjymq::AsyncWorker::ptr awp = std::make_shared<kjymq::AsyncWorker>();
    //2. 实例化连接对象
    kjymq::Connection::ptr conn = std::make_shared<kjymq::Connection>("127.0.0.1", 8085, awp);
    //3. 通过连接创建信道
    kjymq::Channel::ptr channel = conn->openChannel();
    
    //4. 通过信道提供的服务完成所需
    //  a. 声明一个交换机exchange1, 交换机类型为广播模式
    google::protobuf::Map<std::string, std::string> tmp_map;
    channel->declareExchange("exchange1", kjymq::ExchangeType::TOPIC, true, false, tmp_map);
    //  b. 声明一个队列queue1   
    channel->declareQueue("queue1", true, false, false, tmp_map);
    //  c. 声明一个队列queue2
    channel->declareQueue("queue2", true, false, false, tmp_map);
    //  d. 绑定queue1-exchange1，且binding_key设置为queue1
    channel->queueBind("exchange1", "queue1", "queue1");
    //  e. 绑定queue2-exchange1，且binding_key设置为news.music.#
    channel->queueBind("exchange1", "queue2", "news.music.#");
    
    //5. 循环向交换机发布消息
    for(int i = 0; i < 10; i++)
    {
        kjymq::BasicProperties props;
        props.set_id(kjymq::UUIDhelper::UUID());
        props.set_delivery_mode(kjymq::DeliveryMode::DURABLE);
        props.set_routing_key("news.music.pop");
        channel->basicPublish("exchange1", &props, "Hello World-" + std::to_string(i));//当前TOPIC模式下，能拿到
        //channel->basicPublish("exchange1", nullptr, "Hello World-" + std::to_string(i));//FANOUT模式

        std::cout << "one message " << std::endl;
    }
    kjymq::BasicProperties props;
    props.set_id(kjymq::UUIDhelper::UUID());
    props.set_delivery_mode(kjymq::DeliveryMode::DURABLE);
    props.set_routing_key("news.music.sport");
    channel->basicPublish("exchange1", &props, "Hello kkkkkk");//当前TOPIC模式下，能拿到

    props.set_routing_key("news.sport");
    channel->basicPublish("exchange1", &props, "Hello dadadadad");//当前TOPIC模式下，拿不到
    //std::this_thread::sleep_for(std::chrono::seconds(3)); // 临时测试
    //6. 关闭信道
    conn->closeChannel(channel);
    return 0;
}