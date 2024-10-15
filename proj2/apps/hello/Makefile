# Makefile for Cross-Compiling User-Space Application for Raspberry Pi

CC = arm-linux-gnueabihf-gcc  # Cross-compiler for ARM architecture
TARGET = usrhello  # Name of the executable
SRC = usrhello.c  # Source file

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)

