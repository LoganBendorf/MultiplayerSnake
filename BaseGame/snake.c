
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <stdarg.h>


#ifdef _WIN32
#endif

#ifdef __linux__
#endif

#include "gameLogic.h"
#include "errorFunctions.h"
#include "timeHelpers.h"


int main(int argc, char* argv[]) {

    if (argc > 3 || argc == 2) {
        printf("Userage: ./snake width height\n"); exit(1); }

    int WIDTH = 10;
    int HEIGHT = 10;
    if (argc == 3) {
        WIDTH = atoi(argv[1]);
        if (WIDTH < 5) {
            printf("Width too small\n"); exit(1);
        } else if (WIDTH > 250) {
            printf("Width too big\n"); exit(1);}

        HEIGHT = atoi(argv[2]);
        if (HEIGHT < 5) {
            printf("Height too small\n"); exit(1);
        } else if (HEIGHT > 20) {
            printf("Height too big\n"); exit(1);}
    }

    signal(SIGINT , catchSigThenExit);
    signal(SIGABRT , catchSigThenExit);
    signal(SIGILL , catchSigThenExit);
    signal(SIGFPE , catchSigThenExit);
    signal(SIGSEGV, catchSigThenExit); // <-- this one is for segmentation fault
    signal(SIGTERM , catchSigThenExit);

    disableEcho();

    char* map = (char*) Malloc(WIDTH * HEIGHT);
    for (int i = 0; i < HEIGHT; i++) {
        for (int j = 0; j < WIDTH; j++) {
            if (i == 0 || i == HEIGHT - 1 || j == 0 || j == WIDTH - 1) {
                map[i * WIDTH + j] = '#';
            } else {
                map[i * WIDTH + j] = ' '; }
        }
    }
    int pos = HEIGHT/2 * WIDTH + WIDTH/2;
    map[pos] = 'O';

    screenData screen = {map, WIDTH, HEIGHT};
    
    printScreen(screen);
    printf("Starting position = %d\n", pos);

    srand(time(NULL));   // Initialization, should only be called once

    // Player and nodes should probably be on the stack
    node* player = (node*) Malloc(sizeof(node));
    player->xMov = 0;
    player->yMov = 0;
    player->prevXMov = 0;
    player->prevYMov = 0;
    player->xPos = WIDTH / 2;
    player->yPos = HEIGHT / 2;
    player->next = NULL;
    player->hasTail = false;
    player->score = 0;

    printf("size of node = %ld\n", sizeof(node));

    errorInfo errorData = {0, 0, 0};

    struct timespec lastTime, currentTime;

    clock_gettime(CLOCK_MONOTONIC, &lastTime);


    //printf("%ld us\n", diff_micro(&start, &end));
    //printf("%ld ms\n", diff_milli(&start, &end));

    bool appleOnMap = false;
    bool gameStart = false;
    //////////////// MAIN LOOP ////////////////////
    #define TIMER true
    #define DEBUG false
    while (true) {
    #if TIMER == true
        clock_gettime(CLOCK_MONOTONIC, &currentTime);

        long elapsedTime = diffMilli(&lastTime, &currentTime);

        bool shouldBuffer = true;
        if (elapsedTime > 400 && shouldBuffer) {
            getInput(player, shouldBuffer, player->hasTail);}

        if (elapsedTime < 1000) {
            continue;}

        lastTime = currentTime;

        shouldBuffer = false;
        getInput(player, shouldBuffer, player->hasTail);
        if (!gameStart) {
            if (player->xMov == 0 && player->yMov == 0) {
                continue;}
            gameStart = true;
        }
    #else
        int xInput = 0;
        int yInput = 0;
        char input = 0;
        while (xInput == 0 && yInput == 0) {
            input = getch_nonblock();
            xInput = -1 * (input == 'a') + 1 * (input == 'd');
            yInput = -1 * (input == 'w') + 1 * (input == 's');
        }
        player->prevXMov = player->xMov;
        player->prevYMov = player->yMov;

        player->xMov = xInput;
        player->yMov = yInput;
    #endif
        addErrorMsgFormat(errorData, "Player at (%d, %d). Velocity = (%d, %d). PrevMov = (%d, %d)\n", 
                player->xPos, player->yPos, player->xMov, player->yMov, player->prevXMov, player->prevYMov);

        deathCheck(player, screen);

        // Update head
        player->xPos += player->xMov;
        player->yPos += player->yMov;

        updateTailMovement(player, &errorData, &screen);
        
        if (appleOnMap) {
            appleOnMap = addTailPieceIfApple(player, &errorData, screen);
        }

        // Draw head
        screen.map[player->yPos * WIDTH + player->xPos] = 'O';
        if (!player->hasTail) {
            screen.map[(player->yPos - player->yMov) * WIDTH + player->xPos - player->xMov] = ' ';
        }

        drawTail(player, &errorData, &screen);

        while (!appleOnMap) {
            appleOnMap = addApples(&screen);
        }

        printScreen(screen);

        #if DEBUG == true
        printErrorMessages(&errorData);
        #endif
    }
}