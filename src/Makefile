CC:=clang
CFLAGS:=-Wall -Wextra
LIBS:=
SRC:=main.c
HEADERS:=
TARGET:=again

all: $(TARGET)

$(TARGET): $(SRC) $(HEADERS)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)

clean:
	rm -rf $(TARGET)
