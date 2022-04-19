bible:
	mkdir -p ./obj
	cd obj && cc -Wall -ggdb -O0 \
		-c ../src/menu.c ../src/status.c ../src/scripture.c \
		$$(ncursesw5-config --cflags --libs)

	cc -Wall -ggdb -O0 -c src/menu.c      -o obj/menu.o
	cc -Wall -ggdb -O0 -c src/status.c    -o obj/status.o
	cc -Wall -ggdb -O0 -c src/scripture.c -o obj/scripture.o
	cc -Wall -ggdb -O0 -c src/charlie.c   -o obj/charlie.o
	cc -Wall -ggdb -O0 -c src/overtype.c  -o obj/overtype.o
	cc -Wall -ggdb -O0 -c src/file.c  -o obj/file.o
		
	cc -Wall -ggdb -O0 -I../src src/bible.c \
		obj/menu.o obj/status.o obj/scripture.o obj/charlie.o obj/overtype.o obj/file.o \
		-o bible \
		-lm -lutf8proc -lunistring \
		$$(ncursesw5-config --cflags --libs) -lutf8proc 

clean:
	rm -fr overtype bible empty overtype.pid \
		core \
		*.wasm *.js *.data \
		obj/* *~ src/*~

indent:
	indent -kr -ts4 -nut -l80 -br src/*.c

debian:
	apt install libncursesw5-dev libunistring-dev libutf8proc-dev

libutf8proc.bc.o:
	cd ../utf8proc-2.7.0 && emcc utf8proc.c -c -o ../bible-overtype/obj/libutf8proc.bc.o

# Does not work (But would be nice to migrate to xterm)
bible-xterm.html: libutf8proc.bc.o
	echo newcurses
	mkdir -p ./obj
	emcc -I../ncurses-6.1/include src/status.c    -c -o obj/status.bc.o
	emcc -I../ncurses-6.1/include src/menu.c      -c -o obj/menu.bc.o
	emcc -I../ncurses-6.1/include src/scripture.c -c -o obj/scripture.bc.o
	emcc -I../ncurses-6.1/include -I../utf8proc-2.7.0 \
	                   src/charlie.c   -c -o obj/charlie.bc.o
	emcc -I../ncurses-6.1/include -I../utf8proc-2.7.0 \
									   src/overtype.c  -c -o obj/overtype.bc.o
	emcc -I../ncurses-6.1/include src/file.c      -c -o obj/file.bc.o
		
	emcc \
		src/bible.c \
		-L ../em/build/lib \
		-I../ncurses-6.1/build/include/ncurses \
		-I../ncurses-6.1/build/include \
		-I ../utf8proc-2.7.0/ \
		obj/menu.bc.o obj/status.bc.o obj/scripture.bc.o obj/charlie.bc.o obj/overtype.bc.o obj/file.bc.o \
		-lncurses_g \
		--preload-file lib/terminfo@/home/web_user/.terminfo \
		-o bible-xterm.html \
		-g4 \
		-s WASM=1 \
		-s ALLOW_MEMORY_GROWTH=1 \
		--shell-file ./min-shell.html

bible.js:
	mkdir -p ./obj
	emcc -I../emcurses src/status.c    -c -o obj/status.bc.o
	emcc -I../emcurses src/menu.c      -c -o obj/menu.bc.o
	emcc -I../emcurses src/scripture.c -c -o obj/scripture.bc.o
	emcc -I../emcurses -I../utf8proc-2.7.0 \
	                   src/charlie.c   -c -o obj/charlie.bc.o
	emcc -I../emcurses -I../utf8proc-2.7.0 \
									   src/overtype.c  -c -o obj/overtype.bc.o
	emcc -I../emcurses src/file.c      -c -o obj/file.bc.o
		
	emcc -s ALLOW_MEMORY_GROWTH=1 \
	  -I../emcurses -I../utf8proc-2.7.0 src/bible.c \
		obj/menu.bc.o obj/status.bc.o obj/scripture.bc.o obj/charlie.bc.o obj/overtype.bc.o obj/file.bc.o \
		  obj/libutf8proc.bc.o \
		  ../emcurses/emscripten/libpdcurses.so \
		-o bible.js \
		--pre-js ../emcurses/emscripten/termlib.js \
		--preload-file usr/share/bible/
