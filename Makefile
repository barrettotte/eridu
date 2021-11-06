CC = g++
CCFLAGS = -g -Wall
LIBS = 

TARGET = eridu
SRCDIR = src

.PHONY: default all clean

default:	$(TARGET)
all:		default

OBJECTS = $(patsubst $(SRCDIR)/%.cpp, $(SRCDIR)/%.o, $(wildcard $(SRCDIR)/*.cpp) $(wildcard $(SRCDIR)/*/*.cpp))
HEADERS = $(wildcard $(SRCDIR)/*.hpp) $(wildcard $(SRCDIR)/*/*.hpp)

%.o: %.cpp $(HEADERS)
	$(CC) $(CCFLAGS) -c $< -o $@

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) $(LIBS) -o $@

clean:
	-rm -f $(SRCDIR)/*.o
	-rm -f $(TARGET)
