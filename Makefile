CC       ?= gcc
CSTD     ?= c11
WARN     ?= -Wall -Wextra -Werror -pedantic
OPT      ?= -O2
DEFS     ?=
INCLUDES ?= -Iinclude
CFLAGS   ?= $(OPT) -std=$(CSTD) $(WARN) $(DEFS) $(INCLUDES) -MMD -MP
LDFLAGS  ?=
LDLIBS   ?= -pthread -latomic

# Optional sanitizers: make SANITIZE=tsan|asan|ubsan
SANITIZE ?=
ifeq ($(SANITIZE),tsan)
CFLAGS  += -fsanitize=thread -g -O1
LDFLAGS += -fsanitize=thread
endif
ifeq ($(SANITIZE),asan)
CFLAGS  += -fsanitize=address -fno-omit-frame-pointer -g -O1
LDFLAGS += -fsanitize=address
endif
ifeq ($(SANITIZE),ubsan)
CFLAGS  += -fsanitize=undefined -fno-sanitize-recover=all -g
LDFLAGS += -fsanitize=undefined
endif

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

.PHONY: all clean run smoke test

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

# Short run for CI/smoke testing (requires GNU timeout)
RUN_SECS ?= 2
smoke: $(TARGET)
	timeout $(RUN_SECS)s ./$(TARGET) | sed -n '1,20p'

# Basic validation: each generation prints NUM_CONSUMERS lines
test: $(TARGET)
	timeout $(RUN_SECS)s ./$(TARGET) | awk -v n=$$(awk '/#define[ \t]+NUM_CONSUMERS/{print $$3}' include/pc.h) '{ if (match($$0,/frames=([0-9]+)/,m)) c[m[1]]++ } END { for (g in c) if (c[g]!=n) { print "Gen",g,"count",c[g],"(expected",n")"; err=1 } if (!err) print "OK: all gens had",n,"lines" }'

clean:
	rm -rf $(BUILD) $(TARGET)
