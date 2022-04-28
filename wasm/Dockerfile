FROM emscripten/emsdk

RUN apt-get update && apt-get install -y wget
WORKDIR /testspace

#📌 NCurses based on jamesbiv/ncurses-fro-emscripten (not working)

RUN wget https://ftp.gnu.org/pub/gnu/ncurses/ncurses-6.1.tar.gz
RUN wget https://raw.githubusercontent.com/jamesbiv/ncurses-emscripten/master/ncurses-6.1_emscripten.patch
RUN apt-get install -y patch 
RUN tar -xzvf ncurses-6.1.tar.gz
RUN patch -p0 < ncurses-6.1_emscripten.patch


#📌 Native ncurses (We don't need it now)

#WORKDIR /testspace/ncurses-6.1
#RUN apt-get install -y gcc 
#RUN apt-get install -y make 
#RUN ./configure --prefix=/testspace/ncurses-6.1/build
#RUN make && make install

#📌 Emscripten ncurses (does not work)

WORKDIR /testspace/ncurses-6.1
RUN emconfigure ./configure --prefix=/testspace/ncurses-6.1/build
COPY ./ncurses-overrides/Makefile /testspace/ncurses-6.1/Makefile
COPY ./ncurses-overrides/ncurses/Makefile /testspace/ncurses-6.1/ncurses/Makefile
RUN make clean
COPY ./lib /testspace/ncurses-6.1/lib
RUN emmake make && emmake make install

#📌 PDCurses based on rhaberkorn/emcurses (not working)

WORKDIR /testspace
RUN apt-get install -y git 
RUN git clone https://github.com/rhaberkorn/emcurses.git

WORKDIR /testspace/emcurses
RUN ls -las /testspace 
COPY ./emcurses.diff /testspace/emcurses.diff
RUN pwd 
RUN ls -las emscripten
run cat /testspace/emcurses.diff
RUN git apply /testspace/emcurses.diff

#📌 Emscripten PDCurses

WORKDIR /testspace/emcurses/emscripten
RUN emmake make

#📌 Libraries (utf8proc)

WORKDIR /testspace
RUN wget https://github.com/JuliaStrings/utf8proc/archive/refs/tags/v2.7.0.tar.gz \
    -O utf8proc-2.7.0.tar.gz
RUN tar -xzvf utf8proc-2.7.0.tar.gz
WORKDIR /testspace/utf8proc-2.7.0
RUN emcc utf8proc.c -c -o /testspace/libutf8proc.bc.o

# here will be overtype fetching, now we just have it mounted....
