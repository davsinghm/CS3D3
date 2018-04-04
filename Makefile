CXX = clang++
CXXFLAGS = -pthread -std=c++11

#BEGIN

COMPILE = main.o node.o connection.o parser.o graph_vector.o bmf.o
main: $(COMPILE)
	$(CXX) $(COMPILE) -o main

MAIN = main.cpp
main.o: $(MAIN)
	$(CXX) $(CXXFLAGS) $(MAIN) -c main.cpp

CONNECTION = connection.cpp connection.hpp
connection.o: $(CONNECTION)
	$(CXX) $(CXXFLAGS) $(CONNECTION) -c connection.cpp

NODE = node.cpp node.hpp
node.o: $(NODE)
	$(CXX) $(CXXFLAGS) $(NODE) -c node.cpp

PARSER = parser.cpp parser.hpp
parser.o: $(PARSER)
	$(CXX) $(CXXFLAGS) $(PARSER) -c parser.cpp

BF = bf.cpp bf.hpp
bf.o: $(BF)
	$(CXX) $(CXXFLAGS) $(BF) -c bf.cpp

GRAPH_VECTOR = graph_vector.cpp graph_vector.hpp
graph_vector.o: $(GRAPH_VECTOR)
	$(CXX) $(CXXFLAGS) $(GRAPH_VECTOR) -c graph_vector.cpp


#CLEAN UP

clean:
	rm *.o
	rm *.gch
