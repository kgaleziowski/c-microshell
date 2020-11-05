compile: microshell.c
	gcc -Wall -ansi -o microshell microshell.c -lreadline
	clear
	./microshell

delete:
	rm -f *.o microshell

