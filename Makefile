CC       ?= gcc
CSTD     ?= c11
WARN     ?= -Wall -Wextra -Werror -pedantic
OPT      ?= -O2
DEFS     ?=
INCLUDES ?= -Iinclude
CFLAGS   ?= $(OPT) -std=$(CSTD) $(WARN) $(DEFS) $(INCLUDES) -MMD -MP
LDFLAGS  ?=
LDLIBS   ?= -pthread -latomic

TARGET   := traj
SRC_DIR  := src
INC_DIR  := include
BUILD    := build

SRCS := \
  $(SRC_DIR)/main.c \
  $(SRC_DIR)/ring.c \
  $(SRC_DIR)/producer.c \
  $(SRC_DIR)/consumer.c \
  $(SRC_DIR)/topo.c \
  $(SRC_DIR)/frame.c \
  $(SRC_DIR)/util.c

OBJS := $(SRCS:$(SRC_DIR)/%.c=$(BUILD)/%.o)
DEPS := $(OBJS:.o=.d)

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(BUILD)/%.o: $(SRC_DIR)/%.c | $(BUILD)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD):
	@mkdir -p $(BUILD)

-include $(DEPS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BUILD) $(TARGET)
