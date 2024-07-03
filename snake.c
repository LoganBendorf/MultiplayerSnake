
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


int main(int argc, char* argv[]) {

    if (argc > 3 || argc == 2) {
        printf("Userage: ./snake width height\n"); exit(1); }

    int WIDTH = 20;
    int HEIGHT = 20;
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

    clock_t last_time = clock();
    bool appleOnMap = false;
    bool gameStart = false;
    //////////////// MAIN LOOP ////////////////////
    #define TIMER true
    #define DEBUG false
    while (true) {
    #if TIMER == true
        clock_t current_time = clock();
        double elapsed_time = (double)(current_time - last_time) / CLOCKS_PER_SEC;

        bool shouldBuffer = true;
        if (elapsed_time > .4 && shouldBuffer) {
            getInput(player, shouldBuffer, player->hasTail);}

        if (elapsed_time < 1.0) {
            continue;}

        last_time = current_time;

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