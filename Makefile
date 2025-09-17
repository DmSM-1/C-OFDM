CXX = g++
CXXFLAGS = -O3 -g -m64 -mavx -I/usr/include -Iconfig -IOFDM -I/usr/include/python3.10 
LDFLAGS = -lfftw3 -lm -liio -lpython3.10
BUILD = build

# исходники (с путями)
SRCS = main.cpp OFDM/modulation.cpp config/parser.cpp OFDM/Frame.cpp
# объектники — только имена файлов, всё в build/
OBJS = $(addprefix $(BUILD)/,$(notdir $(SRCS:.cpp=.o)))

TARGET = main

.PHONY: all clean

all: $(BUILD) $(TARGET)

$(BUILD):
	mkdir -p $(BUILD)

# где искать cpp-файлы
vpath %.cpp . config OFDM

$(BUILD)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS)

clean:
	rm -rf $(BUILD) $(TARGET)
