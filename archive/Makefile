overtype:
	cc -Wall -ggdb -O0 -o overtype overtype.c -lm -lutf8proc -lunistring $$(ncursesw5-config --cflags --libs)

bible:
	cc -Wall -ggdb -O0 -o bible bible.c $$(ncursesw5-config --cflags --libs)

clean:
	rm -fr overtype bible empty overtype.pid \
		*.wasm *.js *.data \
		obj/* *~ src/*~

indent:
	indent -kr -ts4 -nut -l80 -br *.c src/*.c

debian:
	apt install libncursesw5-dev libunistring-dev libutf8proc-dev

