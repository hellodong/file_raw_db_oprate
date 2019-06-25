

CC:=gcc
BUILD_DIR:=build
OBJS:=$(BUILD_DIR)/main.o
BIN:=modnxp_db
all:$(BUILD_DIR) $(BIN)

$(BUILD_DIR):
	mkdir $@

$(BIN):$(OBJS)
	$(CC) $^ -o $@

$(BUILD_DIR)/%.o:%.c
	$(CC) $< -c -o $@

clean:
	rm $(OBJS) $(BIN)



