overtype:
	cc -Wall -ggdb -O0 -o overtype overtype.c -lm -lutf8proc -lunistring $$(ncursesw5-config --cflags --libs)

bible:
	cc -Wall -ggdb -O0 -o bible bible.c $$(ncursesw5-config --cflags --libs)

empty:
	mkdir -p ./obj

	cc -Wall -ggdb -O0 \
		-c src/menu.c \
		-g -o obj/menu.o \
		$$(ncursesw5-config --cflags --libs)

	cc -Wall -ggdb -O0 \
		empty.c obj/menu.o \
		-o empty \
		$$(ncursesw5-config --cflags --libs)

	# cc -Wall -ggdb -O0 -o bible_ empty.c $$(ncursesw5-config --cflags --libs)

clean:
	rm -f overtype overtype.pid bible *.c~ *.wasm *.js bible_ empty

indent:
	indent -kr -ts4 -nut -l80 *.c

debian:
	apt install libncursesw5-dev libunistring-dev libutf8proc-dev

bible.js:
	emcc -I../emcurses \
		-o bible.js empty.c ../emcurses/emscripten/libpdcurses.so \
		--pre-js ../emcurses/emscripten/termlib.js

rain.js:
	emcc -I../emcurses \
		-o rain.js rain.c ../emcurses/emscripten/libpdcurses.so \
		--pre-js ../emcurses/emscripten/termlib.js
