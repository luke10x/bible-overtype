overtype:
	cc -Wall -ggdb -O0 -o overtype overtype.c -lm -lutf8proc -lunistring $$(ncursesw5-config --cflags --libs)

clean:
	rm -f overtype overtype.pid

indent:
	indent -kr -ts4 -nut -l80 *.c

debian:
	apt install libncursesw5-dev libunistring-dev libutf8proc-dev
