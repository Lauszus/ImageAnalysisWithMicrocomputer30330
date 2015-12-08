CC=g++
CFLAGS+=`pkg-config --cflags opencv`
LDFLAGS+=`pkg-config --libs opencv`

# Link UV4L and WiringPi libraries on ARM
ifneq ($(filter arm%,$(shell uname -m)),)
	CFLAGS+=-I/usr/local/include
	LDFLAGS+=-L/usr/lib/uv4l/uv4lext/armv6l -luv4lext -Wl,-rpath,'/usr/lib/uv4l/uv4lext/armv6l' -L/usr/local/lib -lwiringPi
endif

# Change this to the name of your main file
PROG=main

# Create a list of all object files
OBJS = $(addsuffix .o,$(basename $(notdir $(wildcard *.cpp))))

.PHONY: all clean run

$(PROG): $(OBJS)
	mkdir -p bin
	$(CC) -o bin/$(PROG) $(OBJS) $(LDFLAGS)

%.o: %.cpp
	$(CC) -O3 -c $(CFLAGS) $<

all: $(PROG)

# Remove all objects and bin directory
clean:
	rm -rf $(OBJS) bin

# Run executable inside bin directory
run: all
	sudo ./bin/$(PROG)

# Rebuild everything when makefile changes.
$(OBJS): Makefile
