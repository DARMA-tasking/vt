
CXX=mpic++-mpich-clang
CXXFLAGS=-std=c++14

transport: transport.o collective.o runtime.o
	${CXX} -o $@  $^

clean:
	-rm -rf ${wildcard *.o}

