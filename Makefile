CFLAGS=-Wall -Werror -Wextra -Wpedantic -ggdb -O3
LIBS=-lX11 -lcairo -lXfixes

all:
	gcc $(CFLAGS) $(LIBS) -o main main.c
