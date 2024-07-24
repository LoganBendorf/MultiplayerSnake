all: server client

server: server.c makefile gameLogic.c gameLogic.h errorFunctions.c errorFunctions.h timeHelpers.c timeHelpers.h gameLoop.c gameLoop.h
	gcc -o server server.c gameLogic.c errorFunctions.c timeHelpers.c gameLoop.c -g -lX11 -lXft -lm -I /usr/include/freetype2 -pthread

client: client.c makefile gameLogic.c gameLogic.h errorFunctions.c errorFunctions.h timeHelpers.c timeHelpers.h gameLoop.c gameLoop.h
	gcc -o client client.c gameLogic.c errorFunctions.c timeHelpers.c gameLoop.c -g -lX11 -lXft -lm -I /usr/include/freetype2 -pthread