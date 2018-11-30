CXX = g++
CXXFLAGS = $(shell pkg-config --libs rtrlib) -g


example: example.cpp
	$(CXX) -o example $(CXXFLAGS) example.cpp

clean:
	rm example
