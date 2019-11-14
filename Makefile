TOP_DIR = .
INC_DIR = $(TOP_DIR)/inc
SRC_DIR = $(TOP_DIR)/src
BUILD_DIR = $(TOP_DIR)/build
CC=g++
FLAGS = -pthread -fPIC -g -ggdb -Wall -I$(INC_DIR) -std=c++11
OBJS = $(BUILD_DIR)/tree.o \
	$(BUILD_DIR)/utils.o

default: all
all: test

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	$(CC) $(FLAGS) -c -o $@ $<

test: $(OBJS)
	$(CC) $(FLAGS) $(SRC_DIR)/test.cpp -o test $(OBJS)

clean:
	-rm -f $(BUILD_DIR)/*.o test
