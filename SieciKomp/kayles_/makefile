CXXFLAGS=-Wall -Wextra -Werror -O2 -std=c++20
COMPILER=clang++

.PHONY: all clean
all: kayles_client kayles_server

kayles_client: kayles_client.o err.o lib.o common.o
	$(COMPILER) $(CXXFLAGS) -o $@ $^

kayles_server: kayles_server.o err.o lib.o common.o
	$(COMPILER) $(CXXFLAGS) -o $@ $^

kayles_client.o: kayles_client.cpp
	$(COMPILER) $(CXXFLAGS) -c kayles_client.cpp

kayles_server.o: kayles_server.cpp
	$(COMPILER) $(CXXFLAGS) -c kayles_server.cpp

lib.o: lib.cpp lib.hpp
	$(COMPILER) $(CXXFLAGS) -c lib.cpp

err.o: err.cpp err.hpp
	$(COMPILER) $(CXXFLAGS) -c err.cpp

common.o: common.cpp common.hpp
	$(COMPILER) $(CXXFLAGS) -c common.cpp

clean: 
	rm -f kayles_client kayles_server *.o