CC = cc
CFLAGS = -Wall -Wextra -O2
SRCS = c_src/main.c c_src/util.c c_src/modules.c c_src/ascii.c c_src/themes.c c_src/config.c c_src/renderer.c
TARGET = tonofetch

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET)

.PHONY: clean
