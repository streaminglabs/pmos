CC = clang
CFLAGS = -Wall -O2 -std=c99
SRC = ../../source/pmos.c ../../source/pmos_test.c
TARGET = pmos

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f pmos

.PHONY: all clean
.PHONY: all clean
