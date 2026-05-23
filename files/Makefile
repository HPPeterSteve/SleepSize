CC      = gcc
CFLAGS  = -O2 -Wall -Wextra -D_FORTIFY_SOURCE=2 -fstack-protector-strong \
           $(shell pkg-config --cflags gtk+-3.0)
LDFLAGS = $(shell pkg-config --libs gtk+-3.0)

SRC     = src/main.c src/sleep_core.c src/sleep_fan.c \
          src/sleep_json.c src/sleep_util.c
OUT     = sleepsize

all: $(OUT)

$(OUT): $(SRC)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -f $(OUT)

.PHONY: all clean
