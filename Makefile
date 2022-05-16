myshell: myshell.c
	gcc -g -o myshell myshell.c

clean:
	rm -f *.o
	rm -f myshell
