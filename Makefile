

CC:=gcc
BUILD_DIR:=build
OBJS:=$(BUILD_DIR)/main.o
OBJS +=$(BUILD_DIR)/inter_cmd.o
OBJS +=$(BUILD_DIR)/nxp_devs.o

BIN:=modnxp_db
all:$(BUILD_DIR) $(BIN)

$(BUILD_DIR):
	mkdir $@

$(BIN):$(OBJS)
	$(CC) $^ -o $@

$(BUILD_DIR)/%.o:%.c
	$(CC) $< -g -c -o $@

clean:
	rm $(OBJS) $(BIN)



