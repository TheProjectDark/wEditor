#Detect OS
UNAME_S := $(shell uname -s)

#Compiler selection
ifeq ($(UNAME_S),Linux)
    CXX = g++
endif
ifeq ($(UNAME_S),Darwin)
    CXX = clang++
endif

#Compiler flags
CXXFLAGS = -std=c++23 -O2

TARGET = wEditor

SRC = \
src/MainFrame.cpp \
src/SyntaxHighlighter/SyntaxHighlightCPP.cpp \
src/SyntaxHighlighter/SyntaxHighlightCSharp.cpp \
src/SyntaxHighlighter/SyntaxHighlighter.cpp \
src/SyntaxHighlighter/Text.cpp \
src/SyntaxHighlighter/SyntaxHighlightC.cpp \
src/SyntaxHighlighter/SyntaxHighlightJava.cpp \
src/SyntaxHighlighter/SyntaxHighlightPython.cpp \
src/SyntaxHighlighter/SyntaxHighlightBash.cpp \
src/SyntaxHighlighter/SyntaxHighlightBatch.cpp \
src/SyntaxHighlighter/SyntaxHighlightAssembly.cpp \
src/SyntaxHighlighter/SyntaxHighlightSQL.cpp \
src/DragNDrop.cpp \
src/ThemeSettings.cpp \
src/Preferences.cpp

WX_CONFIG = wx-config

WX_CXXFLAGS = $(shell $(WX_CONFIG) --cxxflags)
WX_LIBS = $(shell $(WX_CONFIG) --libs std,stc)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(WX_CXXFLAGS) -Iinclude $(SRC) -o $(TARGET) $(WX_LIBS)

clean:
	rm -f $(TARGET)