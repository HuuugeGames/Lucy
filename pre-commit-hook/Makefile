CXXFLAGS := -Wall -std=c++17 -fsanitize=address,undefined -ggdb $(shell pkg-config --cflags libgit2)
LDFLAGS := $(shell pkg-config --libs libgit2)

hook : main.cpp
	g++ $(CXXFLAGS) $(LDFLAGS) main.cpp -o hook
