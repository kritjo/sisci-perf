BUILD_DIR = build
LIB_DIR = lib
SRC_DIR = src

#LIB_SRC = $(wildcard $(LIB_DIR)/*.c)
#LIB_OBJS = $(LIB_SRC:$(LIB_DIR)/%.c=$(BUILD_DIR)/%.o)

SRC = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRC:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

TARGET = main

CC = gcc
CFLAGS = -Wall -Werror -I/opt/DIS/include/ -I/opt/DIS/include/dis/ -I$(LIB_DIR)/

all: $(TARGET)

$(TARGET): $(OBJS) #$(LIB_OBJS)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/$@ $^

$(BUILD_DIR)/%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $(BUILD_DIR)/$@

clean:
	rm -rf build/

.PHONY: all clean
