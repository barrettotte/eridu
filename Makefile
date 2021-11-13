CC = g++
CCFLAGS = -g -Wall
LIBS = -lSDL2

OUT = bin
BIN = eridu
SRC = src

OBJECTS = $(patsubst $(SRC)/%.cpp, $(SRC)/%.o, $(wildcard $(SRC)/*.cpp) $(wildcard $(SRC)/*/*.cpp))
HEADERS = $(wildcard $(SRC)/*.hpp) $(wildcard $(SRC)/*/*.hpp)

.PHONY: .FORCE
.FORCE:

all:  clean build

rebuild:  all

%.o: %.cpp $(HEADERS)
	$(CC) $(CCFLAGS) -c $< -o $@

build:	$(OBJECTS)
	@mkdir -p $(OUT)
	$(CC) $(OBJECTS) $(LIBS) -o $(OUT)/$(BIN)

roms:	.FORCE
	@mkdir -p roms
	-curl -o roms.zip "https://www.zophar.net/fileuploads/2/11688yibwo/c8games.zip"
	-unzip roms.zip -d roms
	-rm -f roms.zip

clean:
	@mkdir -p $(OUT)
	-rm -f $(SRC)/*.o
	-rm -f $(OUT)/$(BIN)

test:	all
	./$(OUT)/$(BIN) roms/test_opcode.ch8

tetris:	all
	./$(OUT)/$(BIN) roms/TETRIS

pong: all
	./$(OUT)/$(BIN) roms/PONG2
