run: build
	chmod +x ./testjag
	sudo ./testjag


build: libjaguar.o canutil.o
	g++ -g -DCANDRIVER_SOCKETCAN ./test.c ./libjaguar.c ./canutil.c -lpthread -o ./testjag 

build_prof: libjaguar.o canutil.o ./profile.c
	g++ -g -DCANDRIVER_SOCKETCAN ./profile.c ./libjaguar.o ./canutil.o -lpthread -o ./profiler
	chmod +x ./profiler
 

libjaguar.o canutil.o: ./libjaguar.c ./canutil.c
	g++ -g -DCANDRIVER_SOCKETCAN ./libjaguar.c ./canutil.c -c 
