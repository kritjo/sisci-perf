BUILD_DIR = build
LIB_DIR = lib
SRC_DIR = src

LIB_SRC = $(wildcard $(LIB_DIR)/*.c)
LIB_OBJS = $(LIB_SRC:$(LIB_DIR)/%.c=$(BUILD_DIR)/%.o)

SRC = $(wildcard $(SRC_DIR)/*.c)
OBJS = $(SRC:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)

TARGET = main

CC = gcc
CFLAGS = -Wall -Werror -I/opt/DIS/include/ -I/opt/DIS/include/dis/ -I$(LIB_DIR)/
LDFLAGS = -L/opt/DIS/lib64/ -lsisci

all: $(TARGET)

$(TARGET): $(OBJS) $(LIB_OBJS)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/$@ $^ $(LDFLAGS)

$(OBJS): $(SRC) 
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@  
  
$(LIB_OBJS): $(LIB_SRC) 
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@  

clean:
	rm -rf build/

.PHONY: all clean
