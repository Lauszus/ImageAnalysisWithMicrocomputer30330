CC=g++
CFLAGS+=`pkg-config --cflags opencv`
LDFLAGS+=`pkg-config --libs opencv`

# Change this to the name of your main file
PROG=main

# Create a list of all object files
OBJS = $(addsuffix .o,$(basename $(notdir $(wildcard *.cpp))))

.PHONY: all clean run

$(PROG): $(OBJS)
	mkdir -p bin
	$(CC) -o bin/$(PROG) $(OBJS) $(LDFLAGS)

%.o: %.cpp
	$(CC) -c $(CFLAGS) $<

all: $(PROG)

# Remove all objects and bin directory
clean:
	rm -rf $(OBJS) bin

# Run executable inside bin directory
run: all
	./bin/$(PROG)

# Rebuild everything when makefile changes.
$(OBJS): Makefile