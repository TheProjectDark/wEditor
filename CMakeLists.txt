cmake_minimum_required(VERSION 3.20)
project(wEditor)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Pick wxWidgets root directory based on target architecture.
# Both directories contain STATIC builds produced with:
#   -DwxBUILD_SHARED=OFF -DwxBUILD_MONOLITHIC=OFF
# ---------------------------------------------------------------------------
if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|ARM64")
    message(STATUS "Configuring for ARM64 (wx: C:/msys64/opt/wx-arm64-static)")
    set(CMAKE_PREFIX_PATH "C:/msys64/opt/wx-arm64-static")
else()
    message(STATUS "Configuring for x86-64 (wx: C:/msys64/opt/wx-x64-static)")
    set(wxWidgets_ROOT_DIR "C:/msys64/opt/wx-x64-static")
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|ARM64")
    set(wxWidgets_LIB_DIR        "${wxWidgets_ROOT_DIR}/lib")
    set(wxWidgets_CONFIGURATION  mswu)   # Unicode release; use mswud for debug
endif()

set(wxWidgets_USE_STATIC     ON)
set(wxWidgets_USE_UNICODE    ON)

if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|ARM64")
    set(wxWidgets_USE_DEBUG      OFF)
    set(wxWidgets_USE_UNIVERSAL  OFF)
    set(wxWidgets_FIND_STYLE     win32)
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64|ARM64")
    find_package(wxWidgets CONFIG REQUIRED COMPONENTS core base stc)
else()
    find_package(wxWidgets REQUIRED COMPONENTS core base stc)
    include(${wxWidgets_USE_FILE})
endif()

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------
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

if(WIN32)
    set(WINDOWS_RES app.rc)
endif()

add_executable(wEditor WIN32 ${SOURCES} ${WINDOWS_RES})

target_include_directories(wEditor PRIVATE include)

target_link_libraries(wEditor PRIVATE ${wxWidgets_LIBRARIES})