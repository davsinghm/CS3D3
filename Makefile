CXX = clang++
CXXFLAGS = -pthread -std=c++11

#BEGIN
COMPILE = my-router.o node.o connection.o
my-router: $(COMPILE)
	$(CXX) $(COMPILE) -o my-router

MY-ROUTER = my-router.cpp
my-router.o: $(MY-ROUTER)
	$(CXX) $(CXXFLAGS) $(MY-ROUTER) -c my-router.cpp

CONNECTION = connection.cpp connection.hpp
connection.o: $(CONNECTION)
	$(CXX) $(CXXFLAGS) $(CONNECTION) -c connection.cpp

NODE = node.cpp node.hpp
node.o: $(NODE)
	$(CXX) $(CXXFLAGS) $(NODE) -c node.cpp

#CLEAN UP
clean:
	rm my-router
	rm *.o
	rm *.gch