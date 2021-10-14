overtype:
	cc -Wall -ggdb -O0 -o overtype overtype.c -lm -lutf8proc -lunistring $$(ncursesw5-config --cflags --libs)

bible:
	cc -Wall -ggdb -O0 -o bible bible.c $$(ncursesw5-config --cflags --libs)

empty:
	mkdir -p ./obj
	cd obj && cc -Wall -ggdb -O0 \
		-c ../src/menu.c ../src/status.c ../src/scripture.c \
		$$(ncursesw5-config --cflags --libs)
	
	cc -Wall -ggdb -O0 -I../src\
		empty.c obj/menu.o obj/status.o obj/scripture.o  \
		-o empty \
		$$(ncursesw5-config --cflags --libs)

clean:
	rm -fr overtype bible empty overtype.pid \
		*.wasm *.js *.data \
		obj/* *~ src/*~

indent:
	indent -kr -ts4 -nut -l80 -br *.c src/*.c

debian:
	apt install libncursesw5-dev libunistring-dev libutf8proc-dev

bible.js:
	mkdir -p ./obj
	emcc -I../emcurses src/status.c -c -o obj/status.bc.o
	emcc -I../emcurses src/menu.c -c -o obj/menu.bc.o
	emcc -I../emcurses src/scripture.c -c -o obj/scripture.bc.o
		
	emcc -s ALLOW_MEMORY_GROWTH=1 \
	  -I../emcurses \
		-o bible.js empty.c obj/menu.bc.o obj/status.bc.o obj/scripture.bc.o  \
		  ../emcurses/emscripten/libpdcurses.so \
		--pre-js ../emcurses/emscripten/termlib.js \
		--preload-file usr/share/bible/
