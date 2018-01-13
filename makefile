ftserver : ftserver.c
	gcc -g ftserver.c -o ftserver -lpthread

clean:
	rm ftserver
