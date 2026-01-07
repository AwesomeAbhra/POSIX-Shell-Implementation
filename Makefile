CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -pthread -g
SRCS = main.cpp shell.cpp command.cpp display.cpp handleCD.cpp handleEcho.cpp handleLS.cpp handlePWD.cpp handlepinfo.cpp handlesearch.cpp line_reader.cpp utils_explain.cpp signal.cpp
TARGET = myshell
.PHONY: all clean
all: $(TARGET)
$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) -o $(TARGET)
clean:
	rm -f $(TARGET)