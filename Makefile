CC = gcc
CFLAGS = -Wall -Wextra -Wswitch-enum -I$(INCLUDE_DIR) $(DEBUG_FLAGS)
DEBUG_FLAGS=-ggdb

BUILD_TARGET	=	main
BUILD_DIR		=	build
SRC_DIR			=	src
INCLUDE_DIR		=	include

SRC_FILES 		= 	$(wildcard $(SRC_DIR)/*.c)
OBJ_FILES 		= 	$(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRC_FILES))

$(BUILD_TARGET): $(OBJ_FILES) 
	$(CC) $(CFLAGS) $(OBJ_FILES) $(BUILD_TARGET).c -o $(BUILD_TARGET)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(BUILD_TARGET)