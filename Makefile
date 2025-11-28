CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -fPIC
LDFLAGS =

# Library
LIB_SRC = cmdline.cpp
LIB_OBJ = $(LIB_SRC:.cpp=.o)
LIB_TARGET = libcmdline.so

# Example
EXAMPLE_SRC = example.cpp
EXAMPLE_TARGET = example

# Default target
all: $(LIB_TARGET) $(EXAMPLE_TARGET)

# Build shared library
$(LIB_TARGET): $(LIB_OBJ)
	$(CXX) -shared -o $@ $^ $(LDFLAGS)

# Build example
$(EXAMPLE_TARGET): $(EXAMPLE_SRC) $(LIB_TARGET)
	$(CXX) $(CXXFLAGS) -o $@ $(EXAMPLE_SRC) -L. -lcmdline $(LDFLAGS)

# Compile object files
%.o: %.cpp cmdline.h
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -f $(LIB_OBJ) $(LIB_TARGET) $(EXAMPLE_TARGET)

# Install library (requires sudo)
install: $(LIB_TARGET)
	cp $(LIB_TARGET) /usr/local/lib/
	cp cmdline.h /usr/local/include/
	ldconfig

# Uninstall library (requires sudo)
uninstall:
	rm -f /usr/local/lib/$(LIB_TARGET)
	rm -f /usr/local/include/cmdline.h
	ldconfig

# Run example
run: $(EXAMPLE_TARGET)
	LD_LIBRARY_PATH=. ./$(EXAMPLE_TARGET)

.PHONY: all clean install uninstall run
