all: snake

snake: snake.c makefile gameLogic.c gameLogic.h errorFunctions.c errorFunctions.h timeHelpers.c timeHelpers.h gameLoop.c gameLoop.h host_and_connect.c host_and_connect.h
	gcc -o snake snake.c gameLogic.c errorFunctions.c timeHelpers.c gameLoop.c host_and_connect.c -g -lX11 -lXft -lm -I /usr/include/freetype2 -pthread