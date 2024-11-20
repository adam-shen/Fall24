# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g

# Targets and source files
TARGET = mysh
SRC = mysh.c
OBJ = $(SRC:.c=.o)

# Default target
all: $(TARGET)

# Linking the executable
$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJ)

# Compiling source files to object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Cleaning build files
clean:
	rm -f $(OBJ) $(TARGET)

# Testing the shell
test: $(TARGET)
	@echo "Running test cases..."
	./$(TARGET) < test_script.txt

# Rebuild everything
rebuild: clean all
