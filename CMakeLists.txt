cmake_minimum_required(VERSION 3.20)
project(wEditor)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(wxWidgets CONFIG REQUIRED COMPONENTS core base stc)

add_executable(wEditor WIN32
    src/MainFrame.cpp
    src/DragNDrop.cpp
    src/ThemeSettings.cpp

    src/SyntaxHighlighter/SyntaxHighlighter.cpp
    src/SyntaxHighlighter/SyntaxHighlightCPP.cpp
    src/SyntaxHighlighter/SyntaxHighlightCSharp.cpp
    src/SyntaxHighlighter/SyntaxHighlightC.cpp
    src/SyntaxHighlighter/SyntaxHighlightJava.cpp
    src/SyntaxHighlighter/SyntaxHighlightPython.cpp
    src/SyntaxHighlighter/SyntaxHighlightJavaScript.cpp
    src/SyntaxHighlighter/SyntaxHighlightBash.cpp
    src/SyntaxHighlighter/SyntaxHighlightBatch.cpp
    src/SyntaxHighlighter/SyntaxHighlightAssembly.cpp
    src/SyntaxHighlighter/SyntaxHighlightSQL.cpp
    src/SyntaxHighlighter/CMakeHighlight.cpp
    src/SyntaxHighlighter/MakefileHighlight.cpp
    src/SyntaxHighlighter/Text.cpp

    src/Preferences.cpp
)

target_include_directories(wEditor PRIVATE include)

target_link_libraries(wEditor PRIVATE ${wxWidgets_LIBRARIES})