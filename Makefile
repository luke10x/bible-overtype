bible:
	mkdir -p ./obj
	cd obj && cc -Wall -ggdb -O0 \
		-c ../src/menu.c ../src/status.c ../src/scripture.c \
		$$(ncursesw5-config --cflags --libs)

	cc -Wall -ggdb -O0 -c src/menu.c      -o obj/menu.o
	cc -Wall -ggdb -O0 -c src/status.c    -o obj/status.o
	cc -Wall -ggdb -O0 -c src/scripture.c -o obj/scripture.o
	cc -Wall -ggdb -O0 -c src/charlie.c   -o obj/charlie.o
		
	cc -Wall -ggdb -O0 -I../src src/bible.c \
		obj/menu.o obj/status.o obj/scripture.o obj/charlie.o \
		-o bible \
		$$(ncursesw5-config --cflags --libs) -lutf8proc 

clean:
	rm -fr overtype bible empty overtype.pid \
		*.wasm *.js *.data \
		obj/* *~ src/*~

indent:
	indent -kr -ts4 -nut -l80 -br *.c src/*.c

debian:
	apt install libncursesw5-dev libunistring-dev libutf8proc-dev

libutf8proc.bc.o:
	cd ../utf8proc && emcc utf8proc.c -c -o ../overtype/obj/libutf8proc.bc.o

bible.js: libutf8proc.bc.o
	mkdir -p ./obj
	emcc -I../emcurses src/status.c    -c -o obj/status.bc.o
	emcc -I../emcurses src/menu.c      -c -o obj/menu.bc.o
	emcc -I../emcurses src/scripture.c -c -o obj/scripture.bc.o
	emcc -I../emcurses -I../utf8proc \
	                   src/charlie.c   -c -o obj/charlie.bc.o
		
	emcc -s ALLOW_MEMORY_GROWTH=1 \
	  -I../emcurses -I../utf8proc empty.c \
		obj/menu.bc.o obj/status.bc.o obj/scripture.bc.o obj/charlie.bc.o \
		  obj/libutf8proc.bc.o \
		  ../emcurses/emscripten/libpdcurses.so \
		-o bible.js \
		--pre-js ../emcurses/emscripten/termlib.js \
		--preload-file usr/share/bible/
