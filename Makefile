CC = g++
CFLAGS = -std=c++17 -Wall -Wextra
LIBS = -lcurl -lsqlite3

SRCS = main.cpp account.cpp user.cpp message.cpp call.cpp
OBJS = $(SRCS:.cpp=.o)
TARGET = discord-wrapped

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

%.o: %.cpp
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	$(RM) $(OBJS) $(TARGET)