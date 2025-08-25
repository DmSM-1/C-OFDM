TARGET = main

.PHONY: all clean

all: $(TARGET)

clean:
	rm -rf $(TARGET) *.o
main.o: main.cpp
	g++ -c -o main.o main.cpp

$(TARGET): main.o
	g++ -o $(TARGET) main.o

