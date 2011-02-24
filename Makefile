# Makefile

CC=gcc
CFLAGS=-Wall -O2
SOURCES=resample.c
LIBS=-lsndfile
OBJECTS=${SOURCES:.c=.o}
EXECUTABLE=resample

all: ${SOURCES} ${EXECUTABLE}
    
${EXECUTABLE}: ${OBJECTS}
	${CC} ${CFLAGS} ${LIBS} main.c ${OBJECTS} -o $@

.c.o:
	${CC} ${CFLAGS} ${LIBS} -c $< -o $@


clean:
	rm *.o ${EXECUTABLE}
