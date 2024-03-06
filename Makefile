BUILD_DIR = build
LIB_DIR = lib
SRC_DIR = src

CLIENT_SRC = $(SRC_DIR)/rma.c $(SRC_DIR)/dma.c $(SRC_DIR)/args_parser.c $(SRC_DIR)/client.c
CLIENT_OBJS = $(BUILD_DIR)/rma.o $(BUILD_DIR)/dma.o $(BUILD_DIR)/args_parser.o $(BUILD_DIR)/client.o

SERVER_SRC = $(SRC_DIR)/args_parser.c $(SRC_DIR)/server.c
SERVER_OBJS = $(BUILD_DIR)/args_parser.o $(BUILD_DIR)/server.o

LIB_SRC = $(wildcard $(LIB_DIR)/*.c)
LIB_OBJS = $(LIB_SRC:$(LIB_DIR)/%.c=$(BUILD_DIR)/%.o)

CC = gcc
CFLAGS = -Wall -Werror -I/opt/DIS/include/ -I/opt/DIS/include/dis/ -I$(LIB_DIR)/ -g
LDFLAGS = -L/opt/DIS/lib64/ -lsisci

all: client server

client: $(CLIENT_OBJS) $(LIB_OBJS)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/$@ $^ $(LDFLAGS)

server: $(SERVER_OBJS) $(LIB_OBJS)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/$@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@  
  
$(BUILD_DIR)/%.o: $(LIB_DIR)/%.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@  

clean:
	rm -rf build/

debug_client: all
	valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         ./build/client

debug_server: all
	valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         ./build/server

.PHONY: all clean debug
