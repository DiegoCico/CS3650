BIN=mystery array-max
TEST_DIR=tests
TESTS=$(TEST_DIR)/test_*.sh
EXEC=./mystery

all: $(BIN)

mystery: mystery-main.s libmystery.a
	gcc -g -no-pie mystery-main.s -L. -lmystery -o mystery

array-max: array-max.s array-max-main.c
	gcc -g -no-pie array-max.s array-max-main.c -o array-max

test: all
	@echo "Running tests..."
	@for test in $(TESTS); do \
	    echo "Running $$test..."; \
	    $$test || exit 1; \
	done
	@echo "All tests passed!"

clean:
	rm -f $(BIN)
	rm -f *.o

.PHONY: all test clean
