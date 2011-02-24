# Makefile

CC=gcc
CFLAGS=-Wall -O2
SOURCES=resample.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=resample

all: $(SOURCES) $(EXECUTABLE)
    
$(EXECUTABLE): $(OBJECTS) 
	$(CC) main.c $(OBJECTS) -o $@

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@


clean:
	rm *.o ${EXECUTABLE}
