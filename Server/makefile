server:server.cpp compress.hpp
	g++ -std=c++11 $^ -o $@ -lpthread -lboost_filesystem -lboost_system -lz -lssl -lcrypto
test_server:test_server.cpp
	g++ -std=c++11 $^ -o $@ -lpthread
