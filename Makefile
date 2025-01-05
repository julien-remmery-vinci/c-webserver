PROJECT_NAME = server # Change by yours

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -g

# Directories
SRC_DIR = src
LIB_DIR = lib
BUILD_DIR = build
BIN_DIR = bin
LOGS_DIR = logs
INCLUDE_DIRS = -Iinclude $(shell find $(LIB_DIR) -type d -name include | sed 's/^/-I/')

# Find all .c files in src, lib directories
SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
LIB_SRC_FILES = $(shell find $(LIB_DIR) -type f -name "*.c")

# Output executables in the bin dir
MAIN_TARGET = $(BIN_DIR)/$(PROJECT_NAME)

# Object files
OBJ_FILES = $(patsubst %.c, $(BUILD_DIR)/%.o, $(SRC_FILES) $(LIB_SRC_FILES))

all: $(PROJECT_NAME)

$(PROJECT_NAME): $(OBJ_FILES) | $(BIN_DIR)
	$(CC) $(CFLAGS) $(OBJ_FILES) -o $(MAIN_TARGET)

# Ensure bin/ directory exists
$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

# Compile source files into object files
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INCLUDE_DIRS) -c $< -o $@

run: $(PROJECT_NAME)
	$(BIN_DIR)/$(PROJECT_NAME)

# Clean build and bin files
clean:
	rm -rf $(BUILD_DIR) $(BIN_DIR) $(LOGS_DIR)

# Phony targets
.PHONY: all clean