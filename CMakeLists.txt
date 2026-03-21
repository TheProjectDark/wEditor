cmake_minimum_required(VERSION 3.20)
project(wEditor)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(wxWidgets_ROOT_DIR "C:/wxWidgets-3.3.2/build-mingw")
set(wxWidgets_USE_STATIC ON)
set(wxWidgets_USE_UNICODE ON)

find_package(wxWidgets REQUIRED COMPONENTS core base stc)

include(${wxWidgets_USE_FILE})

set(SOURCES
    src/MainFrame.cpp

    src/DragNDrop.cpp
    src/ThemeSettings.cpp

    src/SyntaxHighlighter/SyntaxHighlighter.cpp
    src/SyntaxHighlighter/SyntaxHighlightCPP.cpp
    src/SyntaxHighlighter/SyntaxHighlightCSharp.cpp
    src/SyntaxHighlighter/SyntaxHighlightC.cpp
    src/SyntaxHighlighter/SyntaxHighlightJava.cpp
    src/SyntaxHighlighter/SyntaxHighlightPython.cpp
    src/SyntaxHighlighter/SyntaxHighlightAssembly.cpp
    src/SyntaxHighlighter/SyntaxHighlightSQL.cpp
    src/SyntaxHighlighter/Text.cpp

    src/Preferences.cpp
)

if(WIN32)
    set(WINDOWS_RES app.rc)
endif()

add_executable(wEditor WIN32 ${SOURCES} ${WINDOWS_RES})

target_include_directories(wEditor PRIVATE include)

target_link_libraries(wEditor ${wxWidgets_LIBRARIES})