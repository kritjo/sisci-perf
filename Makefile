BUILD_DIR = build
LIB_DIR = lib
SRC_DIR = src

SENDER_SRC = $(SRC_DIR)/rma.c $(SRC_DIR)/dma.c $(SRC_DIR)/args_parser.c $(SRC_DIR)/sender.c
SENDER_OBJS = $(BUILD_DIR)/rma.o $(BUILD_DIR)/dma.o $(BUILD_DIR)/args_parser.o $(BUILD_DIR)/sender.o

RECEIVER_SRC = $(SRC_DIR)/args_parser.c $(SRC_DIR)/dma.c $(SRC_DIR)/receiver.c
RECEIVER_OBJS = $(BUILD_DIR)/args_parser.o $(BUILD_DIR)/dma.o $(BUILD_DIR)/receiver.o

LIB_SRC = $(wildcard $(LIB_DIR)/*.c)
LIB_OBJS = $(LIB_SRC:$(LIB_DIR)/%.c=$(BUILD_DIR)/%.o)

CC = gcc
CFLAGS = -Wall -Werror -I/opt/DIS/include/ -I/opt/DIS/include/dis/ -I$(LIB_DIR)/ -g
LDFLAGS = -L/opt/DIS/lib64/ -lsisci

all: sender receiver

sender: $(SENDER_OBJS) $(LIB_OBJS)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/$@ $^ $(LDFLAGS)

receiver: $(RECEIVER_OBJS) $(LIB_OBJS)
	$(CC) $(CFLAGS) -o $(BUILD_DIR)/$@ $^ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@  
  
$(BUILD_DIR)/%.o: $(LIB_DIR)/%.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@  

clean:
	rm -rf build/

debug_sender: all
	valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         ./build/sender

debug_receiver: all
	valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         ./build/receiver

.PHONY: all clean debug
