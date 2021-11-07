CC = g++
CCFLAGS = -g -Wall
LIBS = 

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

clean:
	@mkdir -p $(OUT)
	-rm -f $(SRC)/*.o
	-rm -f $(OUT)/$(BIN)

test:	all
	./$(OUT)/$(BIN) "roms/test.txt"