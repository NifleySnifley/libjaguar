run: build
	chmod +x ./testjag
	sudo ./testjag

build:
	gcc -g -DCANDRIVER_SOCKETCAN ./test.c ./libjaguar.c ./canutil.c -lpthread -o ./testjag 