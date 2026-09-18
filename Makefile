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

ifeq ($(UNAME_S),Darwin)
    MACOS_VERSION_MIN = -mmacosx-version-min=12.0

    ARCH_FLAGS_X86 = -arch x86_64
    ARCH_FLAGS_ARM = -arch arm64
    
    TARGET_X86 = wEditor_x86_64
    TARGET_ARM = wEditor_arm64
else
    TARGET = wEditor
endif

SRC = \
src/MainFrame.cpp \
src/SyntaxHighlighter/SyntaxHighlightCPP.cpp \
src/SyntaxHighlighter/SyntaxHighlightCSharp.cpp \
src/SyntaxHighlighter/SyntaxHighlighter.cpp \
src/SyntaxHighlighter/Text.cpp \
src/SyntaxHighlighter/SyntaxHighlightC.cpp \
src/SyntaxHighlighter/SyntaxHighlightJava.cpp \
src/SyntaxHighlighter/SyntaxHighlightPython.cpp \
src/SyntaxHighlighter/SyntaxHighlightJavaScript.cpp \
src/SyntaxHighlighter/SyntaxHighlightBash.cpp \
src/SyntaxHighlighter/SyntaxHighlightBatch.cpp \
src/SyntaxHighlighter/SyntaxHighlightAssembly.cpp \
src/SyntaxHighlighter/SyntaxHighlightSQL.cpp \
src/SyntaxHighlighter/CMakeHighlight.cpp \
src/SyntaxHighlighter/MakefileHighlight.cpp \
src/DragNDrop.cpp \
src/ThemeSettings.cpp \
src/Preferences.cpp

WX_CONFIG = wx-config

WX_CXXFLAGS = $(shell $(WX_CONFIG) --cxxflags)
WX_LIBS = $(shell $(WX_CONFIG) --libs std,stc)

.PHONY: all clean

ifeq ($(UNAME_S),Darwin)
all: $(TARGET_X86) $(TARGET_ARM)

$(TARGET_X86): $(SRC)
	$(CXX) $(CXXFLAGS) $(MACOS_VERSION_MIN) $(ARCH_FLAGS_X86) $(WX_CXXFLAGS) -Iinclude $(SRC) -o $@ $(WX_LIBS)

$(TARGET_ARM): $(SRC)
	$(CXX) $(CXXFLAGS) $(MACOS_VERSION_MIN) $(ARCH_FLAGS_ARM) $(WX_CXXFLAGS) -Iinclude $(SRC) -o $@ $(WX_LIBS)
else
all: $(TARGET)

$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) $(WX_CXXFLAGS) -Iinclude $(SRC) -o $@ $(WX_LIBS)
endif

clean:
	rm -f $(TARGET) $(TARGET_X86) $(TARGET_ARM)