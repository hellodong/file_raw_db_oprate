

CC:=gcc
BUILD_DIR:=build
OBJS:=$(BUILD_DIR)/main.o
OBJS +=$(BUILD_DIR)/inter_cmd.o
OBJS +=$(BUILD_DIR)/nxp_devs.o
CFLAGS := -g

BIN:=modnxp_db
all:$(BUILD_DIR) $(BIN)

$(BUILD_DIR):
	mkdir $@

$(BIN):$(OBJS)
	$(CC) $^ -o $@

$(BUILD_DIR)/%.o:%.c
	$(CC) $< $(CFLAGS) -c -o $@

clean:
	rm $(OBJS) $(BIN)



