
#ifndef gameLogicFuncts
#define gameLogicFuncts

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef _WIN32
#include <windows.h>
#include <conio.h>
#endif

#ifdef __linux__
#include <unistd.h>
#include <sys/select.h>
#include <termios.h>
#endif

#include "errorFunctions.h"

typedef struct node {
    char xPos;
    char yPos;
    char yMov;
    char xMov;
    char prevXMov;
    char prevYMov;
    struct node* next;
    bool hasTail;
    int score;
} node;

typedef struct screenData {
    char* map;
    char width;
    char height;
} screenData;

char getch_nonblock();

void clearScreen();

void disableEcho();
void enableEcho();

void printScreen(screenData screen);

void gameOver(screenData* screen, char* msg);

void getInput(node* player, bool shouldBuffer, bool playerHasTail);

void updateTailMovement(node* player, errorInfo* errorData, screenData* screen);

bool addTailPieceIfApple(node* player, errorInfo* errorData, screenData screen);

bool addApples(screenData* screen);

void drawTail(node* player, errorInfo* errorData, screenData* screen);

void deathCheck(node* player, screenData screen);

void catchSigThenExit(int sigNum);

#endif