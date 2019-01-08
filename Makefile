CC=g++
CFLAGS=-ggdb -pedantic -Wall -Wextra -Wcast-align -Wcast-qual -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op -Wmissing-declarations -Wmissing-include-dirs -Wnoexcept -Wold-style-cast -Woverloaded-virtual -Wredundant-decls -Wshadow -Wsign-conversion -Wsign-promo -Wstrict-null-sentinel -Wstrict-overflow=5 -Wswitch-default -Wundef -Werror -Wno-unused

all: clean tests

valgrind: clean tests
	valgrind --leak-check=full --show-reachable=yes -v ./tests
	
tests: tests.o SkipList.o
	$(CC) $(CFLAGS) -o $@ $^ -lpthread
	
tests.o: tests.cpp SkipList.o
	$(CC) $(CFLAGS) -c -o $@ $<

SkipList.o: SkipList.cpp
	$(CC) $(CFLAGS) -c -o $@ $<
	
clean:
	rm -rf *.o *.exe