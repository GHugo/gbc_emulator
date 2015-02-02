SRC_DIR=src
LIB_DIR=$(SRC_DIR)/lib

CFLAGS=-Wall -Werror -g -I$(LIB_DIR) -DNDEBUG_MEMORY 
LDFLAGS=-lSDL

all: emulator gbc_file_info

gbc_file_info: $(LIB_DIR)/gbc_format.o $(SRC_DIR)/gbc_file_info.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

emulator: $(SRC_DIR)/emulator.o $(SRC_DIR)/opcodes.o $(SRC_DIR)/gpu.o $(SRC_DIR)/memory.o $(SRC_DIR)/keyboard.o $(SRC_DIR)/timer.o $(SRC_DIR)/interrupts.o $(LIB_DIR)/gbc_format.o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

.PHONY: all clean
clean:
	rm -f $(SRC_DIR)/*.o $(LIB_DIR)/*.o gbc_file_info emulator
