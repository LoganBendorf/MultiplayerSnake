
#include "gameLogic.h"
#include "timeHelpers.h"

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

#define FANCY_GRAPHICS true

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
    int randomX = my_rand() % (screen->width - 2);
    int randomY = my_rand() % (screen->height - 2);
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
void deathCheck(node* player, screenData screen, CLIENT_OR_SERVER cOs) {
    char msg[128] = {0};
    char* clientStr = "Client";
    char* serverStr = "Server";
    if (player->xPos + player->xMov > (screen.width - 2) || player->xPos + player->xMov < 1 ||
                player->yPos + player->yMov > (screen.height - 2) || player->yPos + player->yMov  < 1) {
        sprintf(msg, "%s died. Collision death at (%d, %d) moving into (%d, %d)\n", 
                cOs == CLIENT ? clientStr : serverStr, player->xPos, player->yPos, player->xPos + player->xMov, player->yPos + player->yMov);
        gameOver(&screen, msg);
    }
    // Currently messing with this, can look at BaseGame version for reference
    if (screen.map[(player->yPos + player->yMov) * screen.width + player->xPos + player->xMov] == 'o') {
        if (player->yMov == 0 && player->xMov == 0) {
            //gameOver(&screen, "Zero velocity death\n");
        } else {
            sprintf(msg, "%s died. Collided with self\n", cOs == CLIENT ? clientStr : serverStr);
            gameOver(&screen, msg);
        }
    }
    if (screen.map[(player->yPos + player->yMov) * screen.width + player->xPos + player->xMov] == 'O') {
        if (player->yMov == 0 && player->xMov == 0) {
            //do nothing for now
        } else {
            gameOver(&screen, "Tie\n");
        }
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
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////   FANCY   /////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define FANCY_GRAPHICS true
#if FANCY_GRAPHICS == true
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>

#include <math.h>

#define POSX 750
#define POSY 750
#define WIDTH 500
#define HEIGHT 500
#define BORDER 25
#define LINE 2
#define BLUE "#98edfb"

static Display* display;
static int screen;
static Window rootWindow;
static Visual* visual;
typedef enum {
    blue, black
} COLOR;
COLOR color = blue;


// Use pthread for this
void* fancyInit(void* data) {
    struct threadDataBundle threadData = *((struct threadDataBundle*) data);
    screenData* screenPtr = (screenData*) threadData.screen;
    bool* drawUpdatePtr = (bool*) threadData.drawUpdate;

    XftColor* color_blue = calloc(1, sizeof(XftColor));

    if ((display = XOpenDisplay(NULL)) == NULL) {
        fprintf(stderr, "cant open display\n");
        exit(1);
    }

    screen = DefaultScreen(display);
    rootWindow = RootWindow(display, screen);
    visual = DefaultVisual(display, screen);
    Window main_window = create_window(POSX, POSY, WIDTH, HEIGHT, BORDER);

    create_color(color_blue, BLUE);
    GC gc = create_gc(LINE, color_blue);

    XSizeHints XSH = {.min_width = WIDTH, .min_height = HEIGHT, .max_width = WIDTH, .max_height = HEIGHT};
    XSH.flags = PMinSize | PMaxSize;
    XSetSizeHints(display, main_window, &XSH, XA_WM_NORMAL_HINTS);


    XStoreName(display, main_window, "Snake");

    XMapWindow(display, main_window);

    run(gc, screenPtr, color_blue, drawUpdatePtr);

    //cleanup
    XUnmapWindow(display, main_window);
    XDestroyWindow(display, main_window);
    XftColorFree(display, visual, DefaultColormap(display, screen), color_blue);

    XFreeGC(display, gc);

    XCloseDisplay(display);

    return NULL;
}

void run(GC gc, screenData* screenPtr, XftColor* color_blue, bool* drawUpdatePtr) {

    XEvent ev;

    while (!(XNextEvent(display, &ev))) {

        XSetForeground(display, gc, BlackPixel(display, screen));

        bool shouldDraw = false;

        switch(ev.type) {
 
            case Expose:
                shouldDraw = true;
            case KeyPress:
                if (XkbKeycodeToKeysym(display, ev.xkey.keycode, 0, 0) == XK_q) {
                    return;
                }
                shouldDraw = true;
            default:
                break;
        }

        if (*drawUpdatePtr == true) {
            *drawUpdatePtr = false;
            shouldDraw = true;
        }

        if (shouldDraw) {
            XClearWindow(display, ev.xbutton.window);
            for (int y = 0; y < screenPtr->height; y++) {
                for (int x = 0; x < screenPtr->width; x++) {
                    int squareWidth = WIDTH / screenPtr->width;
                    int squareHeight = HEIGHT / screenPtr->height;
                    switch(screenPtr->map[y * screenPtr->width + x]) {
                        case '#':
                            XSetForeground(display, gc, color_blue->pixel);
                            drawSquare(x * squareWidth, y * squareHeight, squareWidth, squareHeight, ev.xbutton.window, gc);
                            XSetForeground(display, gc, BlackPixel(display, screen));
                            //drawBox(i * (squareWidth - 1), j * (squareHeight - 1), squareWidth, squareHeight, ev.xbutton.window, gc);
                            drawBox(x * squareWidth, y * squareHeight, (squareWidth - 1), (squareHeight - 1), ev.xbutton.window, gc);
                            break;
                        case 'O':
                            drawCircle(x * squareWidth + squareWidth / 2, y * squareHeight + squareHeight / 2, squareWidth / 3, ev.xbutton.window, gc);
                            drawBox(x * squareWidth, y * squareHeight, (squareWidth - 1), (squareHeight - 1), ev.xbutton.window, gc);
                            break;
                        default: 
                            drawBox(x * squareWidth, y * squareHeight, (squareWidth - 1), (squareHeight - 1), ev.xbutton.window, gc);
                            break;
                    }
                }
                printf("\n");
            }
        }
    }
}

void drawCircle(int xStart, int yStart, int radius, Window window, GC gc) {
    if (radius <= 0) {
        printf("Zero or negative radius\n");
        return;
    }

    for (int x = 0; x < radius; x++) {
        int y = round(sqrt(-(x*x) + (radius*radius)));
        XDrawPoint(display, window, gc,  x + xStart,  y + yStart - 1);
        XDrawPoint(display, window, gc,  x + xStart, -y + yStart + 1);
        XDrawPoint(display, window, gc, -x + xStart,  y + yStart - 1);
        XDrawPoint(display, window, gc, -x + xStart, -y + yStart + 1);

        //if (intrp_opt == DONT_INTERPOLATE) {
        //    continue;
        //}

        // Interpolating chat
        int x2 = x+1;
        int y2 = round(sqrt(-(x2*x2) + (radius*radius)));
        float m = (float) (y-y2) / (float) (x-x2);

        if (m < 0) {
            for (int i = 0; i < -m; i++) {
                XDrawPoint(display, window, gc,  x + xStart,  y + yStart - i - 1);
                XDrawPoint(display, window, gc,  x + xStart, -y + yStart + i + 1);
                XDrawPoint(display, window, gc, -x + xStart,  y + yStart - i - 1);
                XDrawPoint(display, window, gc, -x + xStart, -y + yStart + i + 1);
            }
        }
        // Done interpolating
    }
} 

void drawSquare(int xStart, int yStart, int width, int height, Window window, GC gc) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            XDrawPoint(display, window, gc, x + xStart, y + yStart);
        }
    }
}

void drawBox(int xStart, int yStart, int width, int height, Window window, GC gc) {
    for (int y = 0; y < height; y++) {
        if (y == 0 || y == height - 1) {
            for (int x = 0; x < width; x++) {
                XDrawPoint(display, window, gc, x + xStart, y + yStart);
            }
        } else {
            XDrawPoint(display, window, gc, 0 + xStart, y + yStart);
            XDrawPoint(display, window, gc, width - 1 + xStart, y + yStart);
        }
    }
}


void create_color(XftColor* color_blue, const char* name) {

    if (!XftColorAllocName(display, visual, DefaultColormap(display, screen), name, color_blue)) {
        fprintf(stderr, "cant allocate color\n");
        exit(1);
    }

    color_blue->pixel |= 0xff << 24;

     
}

Window create_window(int x, int y, int width, int height, int border) {

    Window window;
    XSetWindowAttributes XWA = {.background_pixel = WhitePixel(display, screen), .border_pixel = BlackPixel(display, screen)};
    XWA.event_mask = Button1MotionMask | ButtonPressMask | ButtonReleaseMask | KeyPressMask;

    window = XCreateWindow(display, rootWindow, x, y, width, height, border,
                            DefaultDepth(display, screen), InputOutput, visual,
                            CWBackPixel | CWBorderPixel | CWEventMask, &XWA);
    return window;
}

GC create_gc(int line_width, XftColor* foreground) {

    GC gc;
    XGCValues x_gc_val;
    unsigned long value_mask;

    x_gc_val.line_style = LineSolid;
    x_gc_val.line_width = line_width;
    x_gc_val.cap_style = CapButt;
    x_gc_val.join_style = JoinMiter;   
    x_gc_val.fill_style = FillSolid;
    x_gc_val.foreground = foreground->pixel;
    x_gc_val.background = WhitePixel(display, screen);

    value_mask = GCForeground | GCBackground | GCFillStyle | GCLineStyle | GCLineWidth | GCCapStyle | GCJoinStyle;
    gc = XCreateGC(display, rootWindow, value_mask,  &x_gc_val);

    return gc;
}
#endif