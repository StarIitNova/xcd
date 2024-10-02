CXX?=g++

TARGET=xcd-a

all: $(TARGET)

$(TARGET): main.cpp
	$(CXX) main.cpp -o $(TARGET)

.phony: $(ALL)
