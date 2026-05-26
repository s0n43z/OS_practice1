CC = gcc
CFLAGS = -Wall -Wextra -pedantic -fPIC
LDFLAGS = -shared

TARGET_LIB = libcaesar.so
TEST_PROG = secure_copy
INSTALL_DIR = /usr/local/lib

all: $(TARGET_LIB) $(TEST_PROG)

$(TARGET_LIB): libcaesar.c libcaesar.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $(TARGET_LIB) libcaesar.c

install:
	sudo cp $(TARGET_LIB) $(INSTALL_DIR)/
	sudo ldconfig

test: $(TARGET_LIB) $(TEST_PROG)
	@echo "Создание тестовой директории и файлов..."
	mkdir -p test_input out_dir
	echo "Содержимое первого файла" > test_input/f1.txt
	echo "Содержимое второго файла" > test_input/f2.txt
	echo "Содержимое третьего файла" > test_input/f3.txt
	echo "Содержимое четвертого файла" > test_input/f4.txt
	echo "Содержимое пятого файла" > test_input/f5.txt
	@echo "Запуск программы..."
	./$(TEST_PROG) test_input/f1.txt test_input/f2.txt test_input/f3.txt test_input/f4.txt test_input/f5.txt out_dir/ 5
	@echo "Содержимое лога:"
	cat log.txt

$(TEST_PROG): secure_copy.c
	$(CC) -pthread -Wall -o $(TEST_PROG) secure_copy.c -ldl

clean:
	rm -f $(TARGET_LIB) $(TEST_PROG) log.txt
	rm -rf test_input out_dir

.PHONY: all install test clean