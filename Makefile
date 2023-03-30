run: build
	chmod +x ./testjag
	sudo ./testjag

build:
	gcc -g ./test.c ./libjaguar.c ./canutil.c -o ./testjag