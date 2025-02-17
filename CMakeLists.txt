cmake_minimum_required(VERSION 3.16)
project(browser)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Находим Qt
find_package(Qt6 REQUIRED COMPONENTS 
    Core 
    Gui 
    Widgets 
    WebEngineWidgets 
    Network
)

# Добавляем пути для поиска заголовочных файлов Qt
include_directories(${Qt6Core_INCLUDE_DIRS}
    ${Qt6Gui_INCLUDE_DIRS}
    ${Qt6Widgets_INCLUDE_DIRS}
    ${Qt6WebEngineWidgets_INCLUDE_DIRS}
    ${Qt6Network_INCLUDE_DIRS}
)

# Добавляем ресурсы
qt_add_resources(RESOURCES
    resources.qrc
)

# Добавляем исполняемый файл
add_executable(browser
    main.cpp
    mainwindow.cpp
    mainwindow.h
    settings.cpp
    settings.h
    types.h
    downloadmanager.cpp
    downloadmanager.h
    downloadsdialog.cpp
    downloadsdialog.h
    readermode.cpp
    readermode.h
    adblocker.cpp
    adblocker.h
    ${RESOURCES}
)

# Линкуем с Qt
target_link_libraries(browser PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::WebEngineWidgets
    Qt6::Network
) 