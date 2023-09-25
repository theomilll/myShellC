all:
	gcc -o shell main.c -pthread

clean:
	rm -f shell
