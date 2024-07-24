

#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <stdio.h>
#include <err.h>

#define POSX 500
#define POSY 500
#define WIDTH 750
#define HEIGHT 500
#define BORDER 25

static Display* display;
static int screen;
static Window rootWindow;

static Window create_window(int x, int y, int width, int height, int border) {
    Window window;
    XSetWindowAttributes XWA;

    XWA.background_pixel = WhitePixel(display, screen);
    XWA.border_pixel = WhitePixel(display, screen);
    XWA.event_mask = ButtonPressMask;


    window = XCreateWindow(display, rootWindow, x, y, width, height, border,
                            DefaultDepth(display, screen), InputOutput, DefaultVisual(display, screen),
                            CWBackPixel | CWBorderPixel | CWEventMask, &XWA);
    return window;
}

static void run() {
    
    XEvent ev;

    while (XNextEvent(display, &ev) == 0) {
        
        switch (ev.type) {
            
            case ButtonPress:
                return;
        }
    }
}

int main() {

    Window wndw;

    if ((display = XOpenDisplay(NULL)) == NULL) {
        errx(1, "Can't open display\n");
    }

    screen = DefaultScreen(display);
    rootWindow = RootWindow(display, screen);

    wndw = create_window(POSX, POSY, WIDTH, HEIGHT, BORDER);

    XMapWindow(display, wndw);

    run();

    //ceanup
    XUnmapWindow(display, wndw);
    XDestroyWindow(display, wndw);
    XCloseDisplay(display);

}