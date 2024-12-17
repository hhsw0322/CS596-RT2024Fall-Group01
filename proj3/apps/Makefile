# Makefile for Cross-Compiling User-Space Application for Raspberry Pi

CC = arm-linux-gnueabihf-gcc  # Cross-compiler for ARM architecture
TARGET = test_project3  # Name of the executable
SRC = test_project3.c  # Source file

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) -o $(TARGET) $(SRC) -no-pie

clean:
	rm -f $(TARGET)
