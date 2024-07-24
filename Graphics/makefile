all: example x11 ex

example: example.c makefile
	gcc -o example example.c -lX11 -lm

x11: x11.c makefile
	gcc -o x11 x11.c -lX11 -lm

ex: example_paint.c makefile
	gcc -o ex example_paint.c -lX11 -lXft -I /usr/include/freetype2

