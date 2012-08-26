CC=g++
FLAGS=-Os
SOURCES=Concurrency.cpp Network.cpp Packet.cpp
HEADERS=$(SOURCES:.cpp=.h)
OBJECTS=$(SOURCES:.cpp=.o)
OUT=dtglib

all: $(OUT)

$(OUT): $(OBJECTS)
	ar -qc lib$(OUT).a $(OBJECTS)

test: $(OUT)
	$(CC) $(FLAGS) -o test test.cpp -L. -l$(OUT) -lpthread

$(OBJECTS) : $(SOURCES) $(HEADERS) Base.h

%.o : %.cpp
	$(CC) $(FLAGS) -c -MMD $< -o $@

clean:
	-rm *.o *.d *.a

.PHONY: clean all
