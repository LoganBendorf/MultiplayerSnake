
#ifndef SNAKE_GRAPHICS
#define SNAKE_GRAPHICS

typedef enum {DO_INTERPOLATE, DONT_INTERPOLATE} INTERPOLATE_OPTIONS;


void init();
void cleanUp();
void waitNextEvent(XEvent* event);
Display** getDisplayPtr();
Window* getWindowPtr();
GC* getGraphicsContextPtr();

void drawSquare(float xStart, float yStart, int width, int height);

void drawLine(int x1, int y1, int x2, int y2, INTERPOLATE_OPTIONS intrp_opt );

void drawA(int x, int y, int scale);

void drawCircle(int xStart, int yStart, int radius, INTERPOLATE_OPTIONS intrp_opt);

#endif