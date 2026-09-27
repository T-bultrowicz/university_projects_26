CC      = gcc
CFLAGS  = -Wall -Wextra -O2 -std=gnu17
LDFLAGS = -Wall

TARGETS = rozglaszacz

all: $(TARGETS)

rozglaszacz: rozglaszacz.o cb.o err.o common.o

cb.o: cb.c cb.h err.h
err.o: err.c err.h
common.o: common.c common.h
rozglaszacz.o: rozglaszacz.c cb.h err.h common.h

.PHONY: all clean

clean:
	rm -f $(TARGETS) *.o
