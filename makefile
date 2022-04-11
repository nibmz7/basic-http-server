# https://stackoverflow.com/questions/30573481/how-to-write-a-makefile-with-separate-source-and-header-directories
SRC_DIR := src
OBJ_DIR := obj
BIN_DIR := bin

EXE := $(BIN_DIR)/startServer
SRC := $(wildcard $(SRC_DIR)/*.c)
OBJ := $(patsubst $(SRC_DIR)/%.c, $(OBJ_DIR)/%.o, $(SRC))

CPPFLAGS := -Iinclude -MMD -MP # -I is a preprocessor flag, not a compiler flag
CFLAGS   := -Wall              # some warnings about bad code
LDFLAGS  := -Llib              # -L is a linker flag
LDLIBS   := #-lm                # Left empty if no libs are needed

all: $(EXE)

.PHONY: all clean

$(EXE): $(OBJ) | $(BIN_DIR) # https://stackoverflow.com/questions/58039810/makefiles-what-is-an-order-only-prerequisite
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BIN_DIR) $(OBJ_DIR):
	mkdir -p $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	@$(RM) -rv $(BIN_DIR) $(OBJ_DIR) # The @ disables the echoing of the command

# Include the .d makefiles. The - at the front suppresses the errors of missing
# Makefiles. Initially, all the .d files will be missing, and we don't want those
# errors to show up.
-include $(OBJ:.o=.d) # The dash is used to silence errors if the files don't exist yet