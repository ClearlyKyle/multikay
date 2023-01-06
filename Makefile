CC = gcc
CFLAGS = -g -Wall -I./kay_pool
LDFLAGS = -lpthread -mwindows

EXE_NAME = pool_test
OBJ_DIR = bin

TEST_SRC := test_kay.c
TEST_OBJ := $(TEST_SRC:.c=.o)

test: $(EXE_NAME)
	$(OBJ_DIR)/$(EXE_NAME)

$(EXE_NAME): $(TEST_OBJ)
	$(CC) $(CFLAGS) -o $(OBJ_DIR)/$@ $(OBJ_DIR)/$^

%.o: %.c $(HEADERS)
	$(CC) $(LDFLAGS) -c $< -o $(OBJ_DIR)/$@

.PHONY: clean
clean:
	rm -f $(OBJ_DIR)/$(EXE_NAME) $(addprefix $(OBJ_DIR)/, $(TEST_OBJ))
