BIN=mystery array-max

all: $(BIN)

mystery: mystery-main.s libmystery.a
	gcc -g -no-pie mystery-main.s -L. -lmystery -o mystery

array-max: array-max.s array-max-main.c
	gcc -g -no-pie array-max.s array-max-main.c -o array-max

clean:
	rm -f $(BIN)
	rm -f *.o

