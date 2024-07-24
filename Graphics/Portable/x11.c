
#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <math.h>
#include <unistd.h>
#include <stdbool.h>

#include <termios.h>

// Other X header should be include below Xlib
#include <X11/Xlib.h>

#include "graphics.h"


char getch_nonblock();

int main() {

    init();

    XEvent event;
    bool drawBruh = false;
    int i = 0;
    sleep(1);
    for (;;) {
        printf("i = %d\n", i++);
        waitNextEvent(&event);
        if (event.type == Expose) {

            if (drawBruh) {
                //XDrawString(display, window, graphicsContext, 100, 100, "Bruh", 4);
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

    cleanUp();
}