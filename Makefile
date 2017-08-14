
CXX=mpic++-mpich-clang
CXXFLAGS=-std=c++14 -g

transport: transport.o collective.o runtime.o event.o termination.o active.o
	${CXX} -o $@  $^

clean:
	-rm -rf ${wildcard *.o}

run: transport
	mpirun-mpich-clang -n 4 ./transport
