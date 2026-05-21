CC = gcc
CFLAGS = -Wall -Wextra -pedantic -fPIC
LDFLAGS = -shared

TARGET_LIB = libcaesar.so
TEST_PROG = secure_copy
SECURE_COPY = secure_copy
INSTALL_DIR = /usr/local/lib

all: $(TARGET_LIB) $(TEST_PROG) $(SECURE_COPY)

$(TARGET_LIB): libcaesar.c libcaesar.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET_LIB) libcaesar.c

install:
	sudo cp $(TARGET_LIB) $(INSTALL_DIR)/
	sudo ldconfig

test: $(TARGET_LIB) $(TEST_PROG)
	./$(TEST_PROG) ./$(TARGET_LIB) 5 input.txt output.txt
	./$(TEST_PROG) ./$(TARGET_LIB) 5 output.txt output2.txt

$(TEST_PROG): secure_copy.c
	$(CC) -o $(TEST_PROG) secure_copy.c -ldl

$(SECURE_COPY): secure_copy.c
	$(CC) -pthread -Wall -o $(SECURE_COPY) secure_copy.c -ldl

clean:
	rm -f $(TARGET_LIB) $(TEST_PROG) $(SECURE_COPY) input.txt output.txt output2.txt

.PHONY: all install test clean