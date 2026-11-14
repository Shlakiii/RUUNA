CXX ?= c++
CPPFLAGS ?=
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
LDFLAGS ?=
LDLIBS ?=

TARGET := ruumake
SOURCES := Source/main.cpp Source/Parser.cpp Source/CodeGenerator.cpp Source/NativeCompiler.cpp
OBJECTS := $(SOURCES:.cpp=.o)
WIN_TARGET := ruumake.exe
WIN_CXX ?= x86_64-w64-mingw32-g++
WIN_OBJECTS := $(SOURCES:.cpp=.win.o)

.PHONY: all clean exe

all: $(TARGET)

$(TARGET): $(OBJECTS)
	$(CXX) $(LDFLAGS) -o $@ $(OBJECTS) $(LDLIBS)

exe: $(WIN_TARGET)

$(WIN_TARGET): $(WIN_OBJECTS)
	$(WIN_CXX) $(LDFLAGS) -o $@ $(WIN_OBJECTS) $(LDLIBS)

Source/%.o: Source/%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -ISource -c -o $@ $<

Source/%.win.o: Source/%.cpp
	$(WIN_CXX) $(CPPFLAGS) $(CXXFLAGS) -ISource -c -o $@ $<

Source/main.o: Source/Parser.h Source/CodeGenerator.h Source/NativeCompiler.h
Source/Parser.o: Source/Parser.h
Source/CodeGenerator.o: Source/CodeGenerator.h
Source/NativeCompiler.o: Source/NativeCompiler.h
Source/main.win.o: Source/Parser.h Source/CodeGenerator.h Source/NativeCompiler.h
Source/Parser.win.o: Source/Parser.h
Source/CodeGenerator.win.o: Source/CodeGenerator.h
Source/NativeCompiler.win.o: Source/NativeCompiler.h

clean:
	$(RM) $(TARGET) $(OBJECTS) $(WIN_TARGET) $(WIN_OBJECTS)
