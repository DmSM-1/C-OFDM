MKLROOT = /opt/intel/oneapi/mkl/latest
CXX = g++
CXXFLAGS = -O3 -m64 -I$(MKLROOT)/include
LDFLAGS = -L$(MKLROOT)/lib/intel64 -Wl,-rpath,$(MKLROOT)/lib/intel64 \
          -lmkl_intel_lp64 -lmkl_sequential -lmkl_core -lpthread -lm -ldl

TARGET = main

.PHONY: all clean

all: $(TARGET)

clean:
	rm -rf $(TARGET) *.o

main.o: main.cpp
	$(CXX) $(CXXFLAGS) -c main.cpp -o main.o

$(TARGET): main.o
	$(CXX) -o $(TARGET) main.o $(LDFLAGS)
