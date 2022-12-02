CC := gcc
CFLAGS := -Wall -Werror -pthread -O

all: sed

clean:
	rm -f *.o sed
