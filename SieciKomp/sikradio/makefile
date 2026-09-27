CXXFLAGS=-Wall -Wextra -Werror -O2 -std=c++20
COMPILER=clang++
LDFLAGS=-lssl -lcrypto

all: sikradio
.PHONY: all clean

sikradio: main.o server_lib.o err.o parser.o
	$(COMPILER) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

main.o: main.cpp parser.hpp lib.hpp
	$(COMPILER) $(CXXFLAGS) -c main.cpp

server_lib.o: server_lib.cpp server_lib.hpp
	$(COMPILER) $(CXXFLAGS) -c server_lib.cpp

err.o: err.cpp err.hpp
	$(COMPILER) $(CXXFLAGS) -c err.cpp

parser.o: parser.cpp parser.hpp
	$(COMPILER) $(CXXFLAGS) -c parser.cpp

clean: 
	rm -f *.o sikradio