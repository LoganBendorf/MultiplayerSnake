
// Need to intall libxft-dev for X11/Xft

#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <math.h>
#include <unistd.h>
#include <stdbool.h>

#include <termios.h>

// Other X header should be include below Xlib
#include <X11/Xlib.h>

typedef enum {DO_INTERPOLATE, DONT_INTERPOLATE} INTERPOLATE_OPTIONS;

int WNDW_POS_X = 1000;
int WNDW_POS_Y = 100;

int WNDW_WIDTH = 500;
int WNDW_HEIGHT = 500;

// Bit per pixel
int WNDW_DEPTH = 1;

Display* display;
Window window;
int screen;
GC graphicsContext;

void drawSquare(float xStart, float yStart, int width, int height) {
    if (xStart < -1 || yStart < -1 || xStart > 1 || yStart > 1) {
        printf("Normalized coordinates outside bounds\n");
        return;
    }

    xStart = (xStart + 1.0) / 2.0;
    yStart = (yStart + 1.0) / 2.0;

    xStart = WNDW_WIDTH *  xStart;
    yStart = WNDW_HEIGHT * yStart;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            XDrawPoint(display, window, graphicsContext, x + xStart, y + yStart); //
        }
    }
}

void drawLine(int x1, int y1, int x2, int y2, INTERPOLATE_OPTIONS intrp_opt ) {

    // Point slope y1 - y2 = m(x1-x2)
    // Slope intercept = y = mx + b
    if (x2 -x1 == 0) {
        // Can also use parametric function for nice thick lines, just decrease the amount t increases i.e. from .002 to .0002
        float xParameter = x2 - x1;
        float yParamater = y2 - y1;

        for(float t = 0; t < 1; t += .002) {
            int x = xParameter * t +x1;
            int y = yParamater * t +y1;
            XDrawPoint(display, window, graphicsContext, x, y);
        }
        return;
    }
    float m = (float) (y1-y2) / (float) (x1-x2);
    int larger = x1;
    int smaller = x2;
    if (x1 < x2) {
        larger = x2;
        smaller = x1;
    }
    for (int x = smaller; x <= larger; x++) {
        int y = round(m * (x - x1) + y1);
        XDrawPoint(display, window, graphicsContext, x, y);

        if (intrp_opt == DONT_INTERPOLATE) {
            continue;
        }

        if (x==smaller){
            continue;
        }
        if (m < 0) {
            for (int i = 0; i < -m; i++) {
                XDrawPoint(display, window, graphicsContext, x, y - i);
            }
        } else {
            for (int i = 0; i < m; i++) {
                XDrawPoint(display, window, graphicsContext, x, y - i);
            }   
        }
    }
}

void drawA(int x, int y, int scale) {
    if (scale <= 0) {
        return;
    }

    // Left line to center
    int x1 = x;
    int y1 = y;
    int x2 = x1 + 5 * scale;
    int y2 = y1 - 10 * scale;
    float xParameter = x2 - x1;
    float yParamater = y2 - y1;

    for(float t = 0; t < 1; t += .0002) {
        int x = xParameter * t +x1;
        int y = yParamater * t +y1;
        XDrawPoint(display, window, graphicsContext, x, y);
    }

    // Right line to center
    x1 += 10 * scale;
    xParameter = x2 - x1;
    yParamater = y2 - y1;

    for(float t = 0; t < 1; t += .0002) {
        int x = xParameter * t +x1;
        int y = yParamater * t +y1;
        XDrawPoint(display, window, graphicsContext, x, y);
    }

    // Middle line
    x1 = x + 2 * scale;
    y1 = y - 4 * scale;
    x2 = x1 + 6 * scale;
    y2 = y1;
    xParameter = x2 - x1;
    yParamater = y2 - y1;

    for(float t = 0; t < 1; t += .002) {
        int x = xParameter * t +x1;
        int y = yParamater * t +y1;
        XDrawPoint(display, window, graphicsContext, x, y);
    }

}

char getch_nonblock();

void drawCircle(int xStart, int yStart, int radius, INTERPOLATE_OPTIONS intrp_opt) {
    if (radius <= 0) {
        printf("Zero or negative radius\n");
        return;
    }

    for (int x = 0; x < radius; x++) {
        int y = round(sqrt(-(x*x) + (radius*radius)));
        XDrawPoint(display, window, graphicsContext,  x + xStart,  y + yStart - 1);
        XDrawPoint(display, window, graphicsContext,  x + xStart, -y + yStart + 1);
        XDrawPoint(display, window, graphicsContext, -x + xStart,  y + yStart - 1);
        XDrawPoint(display, window, graphicsContext, -x + xStart, -y + yStart + 1);

        if (intrp_opt == DONT_INTERPOLATE) {
            continue;
        }

        // Interpolating chat
        int x2 = x+1;
        int y2 = round(sqrt(-(x2*x2) + (radius*radius)));
        float m = (float) (y-y2) / (float) (x-x2);

        if (m < 0) {
            for (int i = 0; i < -m; i++) {
                XDrawPoint(display, window, graphicsContext,  x + xStart,  y + yStart - i - 1);
                XDrawPoint(display, window, graphicsContext,  x + xStart, -y + yStart + i + 1);
                XDrawPoint(display, window, graphicsContext, -x + xStart,  y + yStart - i - 1);
                XDrawPoint(display, window, graphicsContext, -x + xStart, -y + yStart + i + 1);
            }
        }
        // Done interpolating
    }
} 

int main() {

    display = XOpenDisplay(NULL);
    if (display == NULL) {
        printf("Display failed to open\n");
        exit(1);
    }

    screen = DefaultScreen(display);

    window = XCreateSimpleWindow(display, DefaultRootWindow(display), WNDW_POS_X, WNDW_POS_Y, WNDW_WIDTH, WNDW_HEIGHT, WNDW_DEPTH, BlackPixel(display, screen), WhitePixel(display, screen));
    XSelectInput(display, window, ExposureMask | StructureNotifyMask);

    XMapWindow(display, window);
    
    // DefaultGC(display, screen) to get default graphics context
    graphicsContext = XCreateGC(display, window, 0, NULL);


    XEvent event;
    bool drawBruh = false;
    int i = 0;
    for (;;) {
        printf("i = %d\n", i++);
        XNextEvent(display, &event);
        //int input = 0;
        //input = getch_nonblock();
        //if (input != 0) {
        //    printf("input changed\n");
        //    drawBruh = true;
        //}
        if (event.type == Expose) {

            if (drawBruh) {
                XDrawString(display, window, graphicsContext, 100, 100, "Bruh", 4);
            }

            drawLine(120, 200, 90, 90, DO_INTERPOLATE);
            drawLine(110, 200, 90, 90, DO_INTERPOLATE);
            drawLine(100, 200, 90, 90, DO_INTERPOLATE);
            drawLine(90, 200, 90, 90, DO_INTERPOLATE);

            drawLine(0, 0, 400, 390, DO_INTERPOLATE);

            drawLine(0, 30, 400, 30, DO_INTERPOLATE);
            drawA(50, 200, 1);
            drawA(70, 200, 2);
            drawA(90, 200, 3);
            drawA(120, 200, 4);
        }
    }

    XUnmapWindow(display, window);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
}

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