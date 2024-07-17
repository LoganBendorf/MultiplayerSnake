all: server client

server: server.c makefile gameLogic.c gameLogic.h errorFunctions.c errorFunctions.h timeHelpers.c timeHelpers.h
	gcc -o server server.c gameLogic.c errorFunctions.c timeHelpers.c -g 

client: client.c makefile gameLogic.c gameLogic.h errorFunctions.c errorFunctions.h timeHelpers.c timeHelpers.h
	gcc -o client client.c gameLogic.c errorFunctions.c timeHelpers.c -g 