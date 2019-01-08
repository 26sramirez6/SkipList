CC=g++
CFLAGS=-ggdb -pedantic -std=c++11 -Wall -Wextra -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wnoexcept -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 -Wswitch-default -Wundef -Werror -Wno-unused

all: clean tests

valgrind: clean tests
	valgrind --leak-check=full --show-reachable=yes -v ./tests
	
tests: tests.o
	$(CC) $(CFLAGS) -o $@ $<
	
tests.o: tests.cpp SkipList.hpp
	$(CC) $(CFLAGS) -c -o $@ $<
	
clean:
	rm -rf *.o *.exe