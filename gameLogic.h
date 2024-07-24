
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
    // Byte not char
    char width;
    char height;
} screenData;

struct threadDataBundle {
    bool* drawUpdate;
    screenData* screen;
};

typedef enum {
    CLIENT, SERVER
} CLIENT_OR_SERVER;

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

void deathCheck(node* player, screenData screen, CLIENT_OR_SERVER cOs);

void catchSigThenExit(int sigNum);

    #define FANCY_GRAPHICS true
    #if FANCY_GRAPHICS == true
    #include <X11/X.h>
    #include <X11/Xlib.h>
    #include <X11/Xutil.h>
    #include <X11/XKBlib.h>
    #include <X11/Xatom.h>
    #include <X11/Xft/Xft.h>

    void* fancyInit(void* data);
    void run(GC gc, screenData* screenPtr, XftColor* color_blue, bool* drawUpdate);

    void drawCircle(int xStart, int yStart, int radius, Window window, GC gc);
    void drawSquare(int xStart, int yStart, int width, int height, Window window, GC gc);
    void drawBox(int xStart, int yStart, int width, int height, Window window, GC gc);

    void create_color(XftColor* color_blue, const char* name);
    Window create_window(int x, int y, int width, int height, int border);
    GC create_gc(int line_width, XftColor* foreground);
    #endif

#endif