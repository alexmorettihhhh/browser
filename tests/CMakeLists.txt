find_package(Qt6 REQUIRED COMPONENTS 
    Test 
    WebEngineWidgets 
    Network 
    Sql
    TextToSpeech
    Pdf
    PrintSupport
    Positioning
    WebChannel
    Ssl
    Concurrent
    Core5Compat
)

include_directories(
    ${CMAKE_SOURCE_DIR}/src
)

# Добавляем тесты
add_executable(browser_tests
    main_test.cpp
    history_test.cpp
    bookmark_test.cpp
    download_test.cpp
    adblock_test.cpp
    privacy_test.cpp
    reader_test.cpp
    sync_test.cpp
    certificate_test.cpp
)

target_link_libraries(browser_tests PRIVATE
    Qt6::Test
    Qt6::WebEngineWidgets
    Qt6::Network
    Qt6::Sql
    Qt6::TextToSpeech
    Qt6::Pdf
    Qt6::PrintSupport
    Qt6::Positioning
    Qt6::WebChannel
    Qt6::Ssl
    Qt6::Concurrent
    Qt6::Core5Compat
)

# Включаем поддержку MOC для Qt
set_target_properties(browser_tests PROPERTIES
    AUTOMOC ON
    AUTORCC ON
    AUTOUIC ON
)

# Копируем тестовые ресурсы и сертификаты
file(COPY ${CMAKE_SOURCE_DIR}/resources/test DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_SOURCE_DIR}/resources/certs DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# Регистрируем тесты
add_test(NAME browser_tests COMMAND browser_tests)

# Настройка покрытия кода
if(ENABLE_COVERAGE)
    target_compile_options(browser_tests PRIVATE --coverage)
    target_link_options(browser_tests PRIVATE --coverage)
endif()

# Добавляем тестовые данные в ресурсы
qt_add_resources(TEST_RESOURCES
    test_resources.qrc
) 