CXX := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -pthread

TARGET := logAggregator
SRCS := logAggregator.cpp

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRCS)

clean:
	rm -f $(TARGET)

.PHONY: all clean
