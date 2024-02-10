cmake_minimum_required(VERSION 3.5)

project(LedBadgeQt VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        serial.cpp
        serial.h
        log.cpp
        log.h
        badge.h
        image.cpp
        image.h
        webcam.cpp
        webcam.h
        framerate.h
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(LedBadgeQt
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET LedBadgeQt APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(LedBadgeQt SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(LedBadgeQt
            ${PROJECT_SOURCES}
        )
    endif()
endif()

find_package(PNG REQUIRED)
find_package(HDF5 COMPONENTS C CXX HL REQUIRED)
find_package(OpenCV REQUIRED)

target_include_directories(LedBadgeQt PUBLIC ${PNG_INCLUDE_DIR} ${HDF5_INCLUDE_DIRS} ${OpenCV_INCLUDE_DIRS})

set(PNG_SHARED ON CACHE BOOL "" FORCE)
set(PNG_STATIC OFF CACHE BOOL "" FORCE)
set(PNG_EXECUTABLES OFF CACHE BOOL "" FORCE)
set(PNG_TESTS OFF CACHE BOOL "" FORCE)
set(SKIP_INSTALL_ALL OFF CACHE BOOL "" FORCE)

target_link_libraries(LedBadgeQt PRIVATE Qt${QT_VERSION_MAJOR}::Widgets PUBLIC ${MY_EXEC} ${PNG_LIBRARY} ${HDF5_LIBRARIES} ${HDF5_CXX_LIBRARIES} ${OpenCV_LIBS})

set_target_properties(LedBadgeQt PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

install(TARGETS LedBadgeQt
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(LedBadgeQt)
endif()
