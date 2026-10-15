# Host-side tests for the portable game logic.
test: test/test_logic
	./test/test_logic
test/test_logic: test/test_logic.c src/sp_logic.c src/sp_logic.h
	$(CC) -std=c99 -Wall -Wextra -O1 -o $@ test/test_logic.c src/sp_logic.c
clean:
	rm -f test/test_logic
