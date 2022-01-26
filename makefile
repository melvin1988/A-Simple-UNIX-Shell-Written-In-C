#makefile for main
#the filename must be either Makefile or makefile

main: main.o myshell.o command.o token.o
	gcc main.o myshell.o command.o token.o -o main

main.o: main.c myshell.h command.h token.h
	gcc -c main.c

myshell.o: myshell.c myshell.h
	gcc -c myshell.c

command.o: command.c command.h
	gcc -c command.c

token.o: token.c token.h
	gcc -c token.c

clean:
	rm *.o
