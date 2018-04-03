CXX = clang++
CXXFLAGS = -pthread -std=c++11

#BEGIN

MAIN = udp-server.o connection.o thread_server.o -lpthread
main: $(MAIN)
	$(CXX) $(MAIN) -o main

UDP-SERVER = udp-server.cpp
udp-server.o: $(UDP-SERVER)
	$(CXX) $(CXXFLAGS) $(UDP-SERVER) -c udp-server.cpp

CONNECTION = connection.cpp connection.hpp
connection.o: $(CONNECTION)
	$(CXX) $(CXXFLAGS) $(CONNECTION) -c connection.cpp

THREAD_SERVER = thread_server.cpp thread_server.hpp
thread_server.o: $(THREAD_SERVER)
	$(CXX) $(CXXFLAGS) $(THREAD_SERVER) -c thread_server.cpp

#CLEAN UP

clean:
	rm *.o
	rm *.gch
