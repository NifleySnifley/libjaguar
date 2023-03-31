run: build
	chmod +x ./testjag
	sudo ./testjag

build: lib
	gcc -g -DCANDRIVER_SOCKETCAN ./test.c ./libjaguar.c ./canutil.c -lpthread -o ./testjag 

libjaguar.o canutil.o: ./libjaguar.c ./canutil.c
	gcc -DCANDRIVER_SOCKETCAN ./libjaguar.c ./canutil.c -c 