################################################################################
### Makefile                                                                 ###
###                                                                          ###
### Author of this file: Simon Euchner                                       ###
################################################################################


### Variables
CC=gcc
LD=gcc
LIBS=-lwayland-client
OL=-O1
FLAGS=-Wall -Wextra -pedantic
STD=--std=c99
TARGET=./bin/logo


### Run
all: ${TARGET}
	@echo -e "Building process finished successfully\n"
	@echo -e "Running program"
	./bin/logo


### Link
${TARGET}: ./obj/logo.o ./obj/xdg-shell-protocol.o
	${LD} ${FLAGS} ${OL} -o ./bin/logo ./obj/logo.o \
	./obj/xdg-shell-protocol.o ${LIBS}


### Compile

# window.c
./obj/logo.o: ./src/logo.c
	${CC} ${FLAGS} ${OL} -o ./obj/logo.o -c ./src/logo.c

# xdg-shell-protocol.c
./obj/xdg-shell-protocol.o: ./src/xdg-shell-protocol.c
	${CC} ${FLAGS} ${OL} -o ./obj/xdg-shell-protocol.o \
	-c ./src/xdg-shell-protocol.c


### Clean
clean:
	rm ./bin/logo
	rm ./obj/logo.o
	rm ./obj/xdg-shell-protocol.o

.PHONY: clean
