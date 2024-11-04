# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g

# Target executable name
TARGET = words

# Source and object files
SRCS = words.c
OBJS = $(SRCS:.c=.o)

# Default target to build the program
all: $(TARGET)

# Build the target executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Rule for compiling .c files into .o object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up the build files
clean:
	rm -f $(OBJS) $(TARGET)

# Archive for submission
archive: clean
	tar -cvzf p2.tar words.c Makefile README AUTHOR
