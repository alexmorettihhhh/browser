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
    Sql
)

# Добавляем пути для поиска заголовочных файлов Qt
include_directories(${Qt6Core_INCLUDE_DIRS}
    ${Qt6Gui_INCLUDE_DIRS}
    ${Qt6Widgets_INCLUDE_DIRS}
    ${Qt6WebEngineWidgets_INCLUDE_DIRS}
    ${Qt6Network_INCLUDE_DIRS}
    ${Qt6Sql_INCLUDE_DIRS}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Собираем список исходных файлов
file(GLOB_RECURSE SOURCES
    "src/*.cpp"
    "src/*.h"
)

# Добавляем ресурсы
qt_add_resources(RESOURCES
    resources.qrc
)

# Добавляем исполняемый файл
add_executable(browser
    ${SOURCES}
    ${RESOURCES}
)

# Линкуем с Qt
target_link_libraries(browser PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::WebEngineWidgets
    Qt6::Network
    Qt6::Sql
)

# Копируем ресурсы в директорию сборки
add_custom_command(TARGET browser POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/resources $<TARGET_FILE_DIR:browser>/resources
)

# Добавляем тесты
if(BUILD_TESTING)
    enable_testing()
    add_subdirectory(tests)
endif() 