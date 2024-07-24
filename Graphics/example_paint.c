
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/Xatom.h>
#include <X11/Xft/Xft.h>
#include <stdio.h>
#include <err.h>

#define POSX 500
#define POSY 500
#define WIDTH 750
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

static void create_color(XftColor* color_blue, const char* name) {

    if (!XftColorAllocName(display, visual, DefaultColormap(display, screen), name, color_blue)) {
        errx(1, "cant allocat color\n");
    }

    color_blue->pixel |= 0xff << 24;

     
}

static Window create_window(int x, int y, int width, int height, int border) {

    Window window;
    XSetWindowAttributes XWA = {.background_pixel = WhitePixel(display, screen), .border_pixel = BlackPixel(display, screen)};
    XWA.event_mask = Button1MotionMask | ButtonPressMask | ButtonReleaseMask | KeyPressMask;

    window = XCreateWindow(display, rootWindow, x, y, width, height, border,
                            DefaultDepth(display, screen), InputOutput, visual,
                            CWBackPixel | CWBorderPixel | CWEventMask, &XWA);
    return window;
}

static GC create_gc(int line_width, XftColor* foreground) {

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

static void run(GC gc, XftColor* color_blue) {

    XEvent ev;
    int init = 0;
    int prev_x = 0;
    int prev_y = 0;

    while (!(XNextEvent(display, &ev))) {

        if (color == black) {
            XSetForeground(display, gc, BlackPixel(display, screen));
        }
        if (color == blue) {
            XSetForeground(display, gc, color_blue->pixel);
        }

        switch(ev.type) {
 
            case ButtonPress:
                if (ev.xbutton.button == Button1) {
                    printf("Left\n");
                    //color = blue;
                } else if (ev.xbutton.button == Button3) {
                    printf("Right\n");
                    if (color == black) {
                        color = blue;
                    } else {
                        color = black;
                    }
                } 
                //XDrawPoint(display, ev.xbutton.window, gc, ev.xbutton.x, ev.xbutton.y);
                break;
            case MotionNotify:
                if (init != 0) {
                    XDrawLine(display, ev.xbutton.window, gc, prev_x, prev_y, ev.xbutton.x, ev.xbutton.y);
                } else {
                    XDrawPoint(display, ev.xbutton.window, gc, ev.xbutton.x, ev.xbutton.y);
                    init = 1;
                }
                prev_x = ev.xbutton.x;
                prev_y = ev.xbutton.y;
                break;
            case ButtonRelease:
                init = 0;
                break;
            case KeyPress:
                if (XkbKeycodeToKeysym(display, ev.xkey.keycode, 0, 0) == XK_q) {
                    return;
                }
            default:
                break;
        }
    }
}

int main() {

    XftColor* color_blue = calloc(1, sizeof(XftColor));

    if ((display = XOpenDisplay(NULL)) == NULL) {
        errx(1, "cant open display\n");
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


    XStoreName(display, main_window, "Paint");

    XMapWindow(display, main_window);

    run(gc, color_blue);

    //cleanup
    XUnmapWindow(display, main_window);
    XDestroyWindow(display, main_window);
    XftColorFree(display, visual, DefaultColormap(display, screen), color_blue);

    XFreeGC(display, gc);

    XCloseDisplay(display);


}