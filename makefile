all: snake

snake: snake.c makefile gameLogic.c gameLogic.h errorFunctions.c errorFunctions.h
	gcc -o snake snake.c gameLogic.c errorFunctions.c -g 