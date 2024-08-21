CFLAGS=-Wall -Werror -Wextra -Wpedantic -ggdb
LIBS=-lX11 -lcairo -lXfixes

all:
	gcc $(CFLAGS) $(LIBS) -o main main.c
