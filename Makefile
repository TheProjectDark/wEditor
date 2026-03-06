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
CXXFLAGS = -std=c++20 -O2

TARGET = wEditor

SRC = \
main.cpp \
SyntaxHighlighter/SyntaxHighlightCPP.cpp \
SyntaxHighlighter/SyntaxHighlighter.cpp \
SyntaxHighlighter/Text.cpp \
SyntaxHighlighter/SyntaxHighlightC.cpp \
SyntaxHighlighter/SyntaxHighlightJava.cpp \
SyntaxHighlighter/SyntaxHighlightPython.cpp \
SyntaxHighlighter/SyntaxHighlightAssembly.cpp \
SyntaxHighlighter/SyntaxHighlightSQL.cpp \
Functions/DragNDrop.cpp \
Functions/ThemeSettings.cpp \
Preferences/Preferences.cpp

WX_CONFIG = wx-config

WX_CXXFLAGS = $(shell $(WX_CONFIG) --cxxflags)
WX_LIBS = $(shell $(WX_CONFIG) --libs std,stc)

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(WX_CXXFLAGS) $(SRC) -o $(TARGET) $(WX_LIBS)

clean:
	rm -f $(TARGET)