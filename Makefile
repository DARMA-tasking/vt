
CXX=mpic++-mpich-clang
CXXFLAGS=-std=c++14

transport: transport.o collective.o runtime.o event.o
	${CXX} -o $@  $^

clean:
	-rm -rf ${wildcard *.o}

run: transport
	mpirun-mpich-clang -n 4 ./transport
