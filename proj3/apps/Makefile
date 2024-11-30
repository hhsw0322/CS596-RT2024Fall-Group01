# Makefile for Cross-Compiling User-Space Application for Raspberry Pi

CC = arm-linux-gnueabihf-gcc  # Cross-compiler for ARM architecture
TARGET = test_rsv  # Name of the executable
SRC = test_rsv.c  # Source file

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
