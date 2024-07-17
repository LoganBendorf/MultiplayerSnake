
#include "gameLogic.h"

char getch_nonblock() {
    #ifdef __linux__
    struct termios oldt, newt;
    char ch = '\0';

    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    fd_set rfds;
    struct timeval tv;
    FD_ZERO(&rfds);
    FD_SET(STDIN_FILENO, &rfds);
    tv.tv_sec = 0;
    tv.tv_usec = 0;

    if (select(STDIN_FILENO + 1, &rfds, NULL, NULL, &tv) > 0) {
        ch = getchar();
    }

    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
    #endif
    #ifdef _WIN32
    if (_kbhit())
        return _getch();
    else
        return -1;
    #endif
}

void clearScreen() {
    #ifdef _WIN32
    system("cls");
    #else
    system("clear");
    #endif
}

void disableEcho() {
    #ifdef _WIN32
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE); 
    DWORD mode;
    GetConsoleMode(hStdin, &mode);
    mode &= ~ENABLE_ECHO_INPUT;
    SetConsoleMode(hStdin, mode);
    #else
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    tty.c_lflag &= ~ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
    #endif
}

void enableEcho() {
    #ifdef _WIN32
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE); 
    DWORD mode;
    GetConsoleMode(hStdin, &mode);
    mode |= ENABLE_ECHO_INPUT;
    SetConsoleMode(hStdin, mode);
    #else
    struct termios tty;
    tcgetattr(STDIN_FILENO, &tty);
    tty.c_lflag |= ECHO;
    tcsetattr(STDIN_FILENO, TCSANOW, &tty);
    #endif
}

void printScreen(screenData screen) {
    clearScreen();
    for (int i = 0; i < screen.height; i++) {
        for (int j = 0; j < screen.width; j++) {
            printf("%c", screen.map[i * screen.width + j]);
        }
        printf("\n");
    }
}

void gameOver(screenData* screen, char* msg) {
    enableEcho();
    screen->map[0] = 'D';
    screen->map[1] = 'i';
    screen->map[2] = 'e';
    screen->map[3] = 'd';

    printScreen(*screen);
    printf("%s", msg);

    #ifdef _WIN32
    sleep(8000);
    #else
    sleep(8);
    #endif
    exit(1);
}

void getInput(node* player, bool shouldBuffer, bool playerHasTail) {

    char input = 0;
    input = getch_nonblock();

    // Measuring input is separate from assigning player movement to allow for buffering
    int xInput = -1 * (input == 'a') + 1 * (input == 'd');
    int yInput = -1 * (input == 'w') + 1 * (input == 's');

    static int xMovBuffer = 0;
    static int yMovBuffer = 0;
    if (shouldBuffer) {
        if (xInput != 0 || yInput != 0) {
            xMovBuffer = xInput;
            yMovBuffer = yInput;
            return;
        }
        return;
    } else {
        if (xInput == 0 && yInput == 0) {
            xInput = xMovBuffer;
            yInput = yMovBuffer;
        }
    }

    if (xInput == 0 && yInput == 0) {
        player->xMov = player->prevXMov;
        player->yMov = player->prevYMov;
        return;
    }

    player->prevXMov = player->xMov;
    player->prevYMov = player->yMov;

    player->xMov = xInput;
    player->yMov = yInput;

    if (playerHasTail) {
        if (player->xMov == -player->prevXMov) {
            player->xMov = player->prevXMov;
        }
        if (player->yMov == -player->prevYMov) {
            player->yMov = player->prevYMov;
        }
    }
}

void updateTailMovement(node* player, errorInfo* errorData, screenData* screen) { 
    node* tail = player->next;
    node* previous = player;
    while (tail != NULL) {
        //prevmov = mov;
        //mov = previousNodeMov;
        //position += mov;
        tail->prevXMov = tail->xMov;
        tail->prevYMov = tail->yMov;
        tail->xMov = previous->prevXMov;
        tail->yMov = previous->prevYMov;

        addErrorMsgFormat((*errorData), "Tail moved from (%d, %d) to (%d, %d). Velocity = (%d, %d). PrevMov = (%d, %d)\n", 
            tail->xPos, tail->yPos, tail->xPos + tail->xMov, tail->yPos + tail->yMov, tail->xMov, tail->yMov, tail->prevXMov, tail->prevYMov);

        tail->xPos += tail->xMov;
        tail->yPos += tail->yMov;

        previous = tail;
        tail = tail->next;
    }
}

bool addTailPieceIfApple(node* player, errorInfo* errorData, screenData screen) {
    if (screen.map[(player->yPos) * screen.width + player->xPos] == 'a') {
        node* tailPiece = (node*) Malloc(sizeof(node));

        node* endPiece = player;
        while (endPiece->next != NULL) {
            endPiece = endPiece->next;
        }
        endPiece->next = tailPiece;

        //position=endpiecePos - endpieceMov;
        //mov=0; dont think it matters what this one is
        //prevmov=endpieceMov

        tailPiece->xPos = endPiece->xPos - endPiece->xMov;
        tailPiece->yPos = endPiece->yPos - endPiece->yMov;
        tailPiece->xMov = 0;
        tailPiece->yMov = 0;
        tailPiece->prevXMov = endPiece->xMov;
        tailPiece->prevYMov = endPiece->yMov;
        tailPiece->next = NULL;
        tailPiece->score = -1;

        player->hasTail = true;

        addErrorMsgFormat((*errorData), "Tail created at (%d, %d). Velocity = (%d, %d). PrevMov = (%d, %d)\n", 
            tailPiece->xPos, tailPiece->yPos, tailPiece->xMov, tailPiece->yMov, tailPiece->prevXMov, tailPiece->prevYMov);
        return false;
    }
    return true;
}

// ASSUMES WALLS ARE 1 UNIT THICK
bool addApples(screenData* screen) {
    int randomX = rand() % (screen->width - 2);
    int randomY = rand() % (screen->height - 2);
    randomX += 1;
    randomY += 1;

    if (screen->map[randomY * screen->width + randomX] == 'O' || 
            screen->map[randomY * screen->width + randomX] == 'o') {
        return false;
    }

    screen->map[randomY * screen->width + randomX] = 'a';

    return true;
}

void drawTail(node* player, errorInfo* errorData, screenData* screen) {
    node* tail = player->next;
    node* previous = player;
    bool tailDrawn = false;
    while (tail != NULL) {
        screen->map[tail->yPos * screen->width + tail->xPos] = 'o';
        if (tail->next == NULL) {
            screen->map[(tail->yPos - previous->prevYMov) * screen->width + tail->xPos - previous->prevXMov] = ' ';
        }

        tailDrawn = true;
        addErrorMsgFormat((*errorData), "Tail drawn at (%d, %d)\n", tail->xPos, tail->yPos);
        previous = tail;
        tail = tail->next;
    }
    if (!tailDrawn) {
        addErrorMsg((*errorData), "Tail not drawn\n");
    }
}

// ASSUMES WALLS ARE 1 UNIT THICK
void deathCheck(node* player, screenData screen) {
    if (player->xPos + player->xMov > (screen.width - 2) || player->xPos + player->xMov < 1 ||
                player->yPos + player->yMov > (screen.height - 2) || player->yPos + player->yMov  < 1) {
        char msg[128] = {0};
        sprintf(msg, "Collision death at (%d, %d) moving into (%d, %d)\n", 
                player->xPos, player->yPos, player->xPos + player->xMov, player->yPos + player->yMov);
        gameOver(&screen, msg);
    }
    if (screen.map[(player->yPos + player->yMov) * screen.width + player->xPos + player->xMov] == 'o' ||
            screen.map[(player->yPos + player->yMov) * screen.width + player->xPos + player->xMov] == 'O') {
        if (player->yMov == 0 && player->xMov == 0) {
            gameOver(&screen, "Zero velocity death\n");
        }
        gameOver(&screen, "Collided with self\n");
    }
}

// Probably should be somewhere else
void catchSigThenExit(int sigNum) {
    if (sigNum == 2) {
        printf ("\nInterupt Signal received\n");
        enableEcho();
        exit(1);
    }
    printf ("\nSignal %d received\n",sigNum);
    enableEcho();
    exit(1);
}