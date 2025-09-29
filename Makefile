CXX = g++
CXXFLAGS = -O3 -g -m64 -I/usr/include -Iconfig -IOFDM -I/usr/include/python3.10 
LDFLAGS = -lfftw3 -lm -liio -lpython3.10 -fopenmp 
BUILD = build

# исходники (с путями)
SRCS = main.cpp tx.cpp rx.cpp st_rx.cpp OFDM/modulation.cpp config/parser.cpp OFDM/Frame.cpp
# объектники — только имена файлов, всё в build/
OBJS = $(addprefix $(BUILD)/,$(notdir $(SRCS:.cpp=.o)))

TARGETS = main tx rx st_rx

.PHONY: all clean

all: $(BUILD) $(TARGETS)

$(BUILD):
	mkdir -p $(BUILD)

# где искать cpp-файлы
vpath %.cpp . config OFDM

$(BUILD)/%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# отдельные таргеты
main: $(BUILD)/main.o $(BUILD)/modulation.o $(BUILD)/parser.o $(BUILD)/Frame.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

tx: $(BUILD)/tx.o $(BUILD)/modulation.o $(BUILD)/parser.o $(BUILD)/Frame.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

rx: $(BUILD)/rx.o $(BUILD)/modulation.o $(BUILD)/parser.o $(BUILD)/Frame.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)


st_rx: $(BUILD)/st_rx.o $(BUILD)/modulation.o $(BUILD)/parser.o $(BUILD)/Frame.o
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

clean:
	rm -rf $(BUILD) $(TARGETS)
