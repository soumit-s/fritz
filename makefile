CC=gcc
PROG=fritz
SRC_DIR=src
BUILD_DIR=build
OUTPUT_DIR=dist
INCLUDE_DIRS=include

IFLAGS=${INCLUDE_DIRS:%=-I%}
LDFLAGS=-ldl

CFILES=$(shell find ${SRC_DIR} -path ./src/execs -prune -o  -type f | grep .c)
ECFILES=$(shell find ${SRC_DIR}/execs -type f | grep .c)
OFILES=${CFILES:$(SRC_DIR)/%=$(BUILD_DIR)/%.o}

EFILES=${ECFILES:${SRC_DIR}/execs/%.c=${OUTPUT_DIR}/%}

all: ${PROG} ${EFILES} clean

${PROG}:${OFILES}
	${CC} -g ${OFILES} -shared -o ${OUTPUT_DIR}/lib${PROG}.so ${LDFLAGS}

${BUILD_DIR}/%.o:${SRC_DIR}/%
	@# Create the folder if it does not exist
	@if [ ! -e $(shell dirname $@) ]; then mkdir -p $(shell dirname $@); fi
	${CC} -c -g $? -fPIC -o $@ ${IFLAGS}



${OUTPUT_DIR}/% : ${ECFILES}
	${CC} -L./dist -Wl,-rpath=./dist $? -o $@ ${IFLAGS} -lfritz


clean:
	rm -r ${BUILD_DIR}/*
