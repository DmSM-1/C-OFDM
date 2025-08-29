CXX = g++
CXXFLAGS = -O3 -m64 -mavx -Wall -I/usr/include
LDFLAGS = -lfftw3 -lm
BUILD = build
SRCS = main.cpp modulation.cpp parser.cpp
OBJS = $(addprefix $(BUILD)/,$(SRCS:.cpp=.o))
TARGET = main

.PHONY: all clean

all: $(BUILD) $(TARGET)

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LDFLAGS)

clean:
	rm -rf $(BUILD) $(TARGET)
