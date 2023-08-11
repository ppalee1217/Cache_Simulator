# Specify the source files and their corresponding object files
SRCS = queue.c lrustack.c mshr.c cacheset.c cachebank.c main.c
OBJS = $(patsubst %.c, build/%.o, $(SRCS))

# Default target: build cachesim executable
all: cachesim

# Create the build folder if it doesn't exist
build:
	mkdir -p build

# Rule to compile C source files into object files
build/%.o: %.c | build
	g++ -pthread -c $< -o $@

# Rule to link object files and build the cachesim executable
cachesim: $(OBJS)
	g++ $(OBJS) -pthread -lrt -o cachesim

# Clean rule to remove generated files
clean:
	rm -rf build cachesim