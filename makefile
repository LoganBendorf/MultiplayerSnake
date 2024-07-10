all: server client

server: server.c makefile gameLogic.c gameLogic.h errorFunctions.c errorFunctions.h
	gcc -o server server.c gameLogic.c errorFunctions.c -g 

client: client.c makefile gameLogic.c gameLogic.h errorFunctions.c errorFunctions.h
	gcc -o client client.c gameLogic.c errorFunctions.c -g 