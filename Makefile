CC = gcc
CFLAGS = -Wall -Wextra -pedantic -fPIC
LDFLAGS = -shared
TARGET_LIB = libcaesar.so
TEST_PROG = test_program
INSTALL_DIR = /usr/local/lib

all: $(TARGET_LIB)

$(TARGET_LIB): libcaesar.c libcaesar.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET_LIB) libcaesar.c

install:
	sudo cp $(TARGET_LIB) $(INSTALL_DIR)/
	sudo ldconfig

test: $(TARGET_LIB) $(TEST_PROG)
	./$(TEST_PROG) ./$(TARGET_LIB) 42 input.txt output.txt
	./$(TEST_PROG) ./$(TARGET_LIB) 42 output.txt output2.txt

$(TEST_PROG): test_program.c
	$(CC) -o $(TEST_PROG) test_program.c -ldl

clean:
	rm -f $(TARGET_LIB) $(TEST_PROG) input.txt output.txt output2.txt

.PHONY: all install test clean