cmake_minimum_required(VERSION 3.20)
project(wEditor)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#path to wxwidgets
set(wxWidgets_ROOT_DIR "C:/wxWidgets-3.3.2/build-mingw")
set(wxWidgets_USE_STATIC ON)
set(wxWidgets_USE_UNICODE ON)

#wxwidgets lib
find_package(wxWidgets REQUIRED COMPONENTS core base stc)

include(${wxWidgets_USE_FILE})

#source
set(SOURCES
    MainFrame.cpp

    Functions/DragNDrop.cpp
    Functions/ThemeSettings.cpp

    SyntaxHighlighter/SyntaxHighlighter.cpp
    SyntaxHighlighter/SyntaxHighlightCPP.cpp
    SyntaxHighlighter/SyntaxHighlightCSharp.cpp
    SyntaxHighlighter/SyntaxHighlightC.cpp
    SyntaxHighlighter/SyntaxHighlightJava.cpp
    SyntaxHighlighter/SyntaxHighlightPython.cpp
    SyntaxHighlighter/SyntaxHighlightAssembly.cpp
    SyntaxHighlighter/SyntaxHighlightSQL.cpp
    SyntaxHighlighter/Text.cpp

    Preferences/Preferences.cpp
)

if(WIN32)
    set(WINDOWS_RES app.rc)
endif()

add_executable(wEditor WIN32 ${SOURCES} ${WINDOWS_RES})

target_link_libraries(wEditor ${wxWidgets_LIBRARIES})