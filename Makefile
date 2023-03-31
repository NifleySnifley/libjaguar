run: build
	chmod +x ./testjag
	sudo ./testjag

build:
	gcc -g -DCANDRIVER_SERIAL ./test.c ./libjaguar.c ./canutil.c -o ./testjag