################################################################################
### Makefile to build the program *./bin/logo*                               ###
################################################################################

CC=gcc
LD=gcc
LIBS=-lwayland-client
OL=-O1
FLAGS=-Wall -Wextra -pedantic
STD=--std=c99
TARGET=./bin.d/logo

### Run
all: ${TARGET}
	@echo -e "Building process finished successfully\n"
	@echo -e "Running program"
	./bin.d/logo

### Link
${TARGET}: ./obj.d/logo.o ./obj.d/xdg-shell-protocol.o
	${LD} ${FLAGS} ${OL} -o ./bin.d/logo ./obj.d/logo.o \
	./obj.d/xdg-shell-protocol.o ${LIBS}

### Compile

# window.c
./obj.d/logo.o: ./src.d/logo.c
	${CC} ${FLAGS} ${OL} -o ./obj.d/logo.o -c ./src.d/logo.c

# xdg-shell-protocol.c
./obj.d/xdg-shell-protocol.o: ./src.d/xdg-shell-protocol.c
	${CC} ${FLAGS} ${OL} -o ./obj.d/xdg-shell-protocol.o \
	-c ./src.d/xdg-shell-protocol.c

### Clean
clean:
	rm ./bin.d/logo
	rm ./obj.d/logo.o
	rm ./obj.d/xdg-shell-protocol.o

.PHONY: clean