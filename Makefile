CC = gcc
CFLAGS = -Wall -Wextra -Wswitch-enum $(DEBUG_FLAGS)

DEBUG_FLAGS = -ggdb

BUILD_TARGET        = main
BUILD_DIR           = build
SRC_DIR             = src
INCLUDE_DIR			= include
ROUTES_DIR          = routes
MIDDLEWARES_DIR     = middlewares

SRC_FILES           = $(wildcard $(SRC_DIR)/*.c)
ROUTES_SRC_FILES    = $(wildcard $(ROUTES_DIR)/**/*.c)
MIDDLEWARES_SRC_FILES = $(wildcard $(MIDDLEWARES_DIR)/**/*.c)

OBJ_FILES           = $(patsubst $(SRC_DIR)/%.c, $(BUILD_DIR)/%.o, $(SRC_FILES))
OBJ_FILES          += $(patsubst $(ROUTES_DIR)/%.c, $(BUILD_DIR)/routes/%.o, $(ROUTES_SRC_FILES))
OBJ_FILES          += $(patsubst $(MIDDLEWARES_DIR)/%.c, $(BUILD_DIR)/middlewares/%.o, $(MIDDLEWARES_SRC_FILES))

INCLUDE_DIRS        = $(INCLUDE_DIR) $(sort $(dir $(ROUTES_SRC_FILES) $(MIDDLEWARES_SRC_FILES)))
CFLAGS             += $(addprefix -I, $(INCLUDE_DIRS))

BUILD_DIRS = $(sort $(dir $(OBJ_FILES)))

all: $(BUILD_DIRS) $(BUILD_TARGET)

$(BUILD_DIRS):
	mkdir -p $@

$(BUILD_TARGET): $(BUILD_DIRS) $(OBJ_FILES)
	$(CC) $(CFLAGS) $(OBJ_FILES) $(BUILD_TARGET).c -o $(BUILD_TARGET)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c | $(BUILD_DIRS)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/routes/%.o: $(ROUTES_DIR)/%.c | $(BUILD_DIRS)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/middlewares/%.o: $(MIDDLEWARES_DIR)/%.c | $(BUILD_DIRS)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)
	rm -f $(BUILD_TARGET)
