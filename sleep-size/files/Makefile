CC      = gcc
CFLAGS  = -O2 -Wall -Wextra -D_FORTIFY_SOURCE=2 -fstack-protector-strong \
           $(shell pkg-config --cflags gtk+-3.0)

LDFLAGS = $(shell pkg-config --libs gtk+-3.0) -lseccomp

SRC     = main.c sleep_core.c sleep_fan.c \
          sleep_json.c sleep_util.c sleep_seccomp.c \
          sleep_state.c sleep_ram.c
OUT     = sleepsize
all: $(OUT)
$(OUT): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
clean:
	rm -f $(OUT)
.PHONY: all clean
