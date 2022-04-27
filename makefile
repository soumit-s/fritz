CC=gcc
PROG=fritz
SRC_DIR=src
BUILD_DIR=build
OUTPUT_DIR=dist
INCLUDE_DIRS=include

IFLAGS=${INCLUDE_DIRS:%=-I%}

CFILES=$(shell find ${SRC_DIR} -type f | grep .c)
OFILES=${CFILES:$(SRC_DIR)/%=$(BUILD_DIR)/%.o}

all: ${PROG} clean

${PROG}:${OFILES}
	${CC} -g ${OFILES} -o ${OUTPUT_DIR}/${PROG}

${BUILD_DIR}/%.o:${SRC_DIR}/%
	@# Create the folder if it does not exist
	@if [ ! -e $(shell dirname $@) ]; then mkdir -p $(shell dirname $@); fi
	${CC} -c -g $? -o $@ ${IFLAGS}

clean:
	rm -r ${BUILD_DIR}/*
