.PHONY:mq_connection

mq_connection:mq_connectiontest.cpp  ../mqcommon/mq_msg.pb.cc ../mqcommon/mq_proto.pb.cc
	g++ -o $@ $^ -std=c++11 -lgtest -lprotobuf -pthread -lsqlite3 -I../mqthird/include


mq_channeltest:mq_channeltest.cpp  ../mqcommon/mq_msg.pb.cc ../mqcommon/mq_proto.pb.cc
	g++ -o $@ $^ -std=c++11 -lgtest -lprotobuf -pthread -lsqlite3 -I../mqthird/include


mq_consumertest:mq_consumertest.cpp  ../mqcommon/mq_msg.pb.cc
	g++ -o $@ $^ -std=c++11 -lgtest -lprotobuf -pthread -lsqlite3

mq_route:mq_routetest.cpp  ../mqcommon/mq_msg.pb.cc
	g++ -o $@ $^ -std=c++11 -lgtest -lprotobuf -pthread -lsqlite3

mq_host:mq_hosttest.cpp  ../mqcommon/mq_msg.pb.cc
	g++ -o $@ $^ -std=c++11 -lgtest -lprotobuf -pthread -lsqlite3

mq_msg:mq_msgtest.cpp  ../mqcommon/mq_msg.pb.cc
	g++ -o $@ $^ -std=c++11 -lgtest -lprotobuf -pthread -lsqlite3

mq_bindtest:mq_bindingtest.cpp
	g++ -o $@ $^ -std=c++11 -lgtest -lprotobuf -pthread -lsqlite3

mq_queuetest:mq_queuetest.cpp
	g++ -o $@ $^ -std=c++11 -lgtest -lprotobuf -pthread -lsqlite3

mq_exchangetest:mq_exchangetest.cpp
	g++ -o $@ $^ -std=c++11 -lgtest -lprotobuf -pthread -lsqlite3

ts:mq_filetest.cpp
	g++ -o $@ $^ -std=c++11
	
.PHONY:clean
clean:
	rm -f ts 
	rm -f mq_exchangetest 
	rm -f mq_queuetest 
	rm -f mq_bindtest 

