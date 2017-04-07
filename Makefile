CXX = g++
CXXFLAGS =-std=c++11

vfs: main.o vfs.o
	$(CXX) main.o vfs.o -o vfs $(CXXFLAGS)

main.o: main.cpp vfs.h
	$(CXX) -c main.cpp -o main.o $(CXXFLAGS)

vfs.o: vfs.cpp vfs.h
	$(CXX) -c vfs.cpp -o vfs.o $(CXXFLAGS)

clean:
	rm -f *.o