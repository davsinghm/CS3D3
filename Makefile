CXX = clang++
CXXFLAGS = -pthread -std=c++11

#BEGIN
COMPILE = main.o node.o connection.o 
main: $(COMPILE)
	$(CXX) $(COMPILE) -o my-router

MAIN = main.cpp
main.o: $(MAIN)
	$(CXX) $(CXXFLAGS) $(MAIN) -c main.cpp

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