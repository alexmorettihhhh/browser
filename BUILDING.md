# Инструкция по сборке Qt Web Browser

## Подготовка среды разработки

### Windows

1. Установите Visual Studio 2019 или новее
2. Установите Qt 6.2 или новее
3. Установите CMake 3.16 или новее
4. Установите Git

```bash
# Клонирование репозитория
git clone https://github.com/yourusername/browser.git
cd browser

# Создание директории для сборки
mkdir build
cd build

# Конфигурация и сборка
cmake -G "Visual Studio 16 2019" -A x64 ..
cmake --build . --config Release
```

### Linux

1. Установите необходимые пакеты:

Ubuntu/Debian:
```bash
sudo apt update
sudo apt install build-essential cmake qt6-base-dev qt6-webengine-dev libssl-dev
```

Fedora:
```bash
sudo dnf install gcc-c++ cmake qt6-qtbase-devel qt6-qtwebengine-devel openssl-devel
```

2. Сборка:
```bash
# Клонирование репозитория
git clone https://github.com/yourusername/browser.git
cd browser

# Создание директории для сборки
mkdir build
cd build

# Конфигурация и сборка
cmake ..
make -j$(nproc)
```

### macOS

1. Установите Xcode
2. Установите Homebrew
3. Установите зависимости:

```bash
brew install cmake qt@6 openssl
```

4. Сборка:
```bash
# Клонирование репозитория
git clone https://github.com/yourusername/browser.git
cd browser

# Создание директории для сборки
mkdir build
cd build

# Конфигурация и сборка
cmake ..
make -j$(sysctl -n hw.ncpu)
```

## Опции сборки

### Общие опции CMake

- `-DCMAKE_BUILD_TYPE=Release|Debug` - тип сборки
- `-DCMAKE_INSTALL_PREFIX=/usr` - путь установки
- `-DBUILD_TESTS=ON|OFF` - сборка тестов
- `-DBUILD_DOCS=ON|OFF` - сборка документации
- `-DENABLE_WEBGL=ON|OFF` - поддержка WebGL
- `-DENABLE_PROPRIETARY_CODECS=ON|OFF` - поддержка проприетарных кодеков

### Пример использования опций

```bash
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DENABLE_WEBGL=ON ..
```

## Решение проблем

### Windows

1. Ошибка "Qt not found":
   - Убедитесь, что переменная Qt6_DIR указывает на правильный путь
   - Используйте -DCMAKE_PREFIX_PATH=путь/к/Qt6

2. Ошибка линковки:
   - Проверьте совместимость версий MSVC и Qt
   - Убедитесь, что все DLL доступны

### Linux

1. Ошибка "QtWebEngine not found":
   - Установите пакет qt6-webengine-dev
   - Проверьте версию Qt

2. Ошибка компиляции:
   - Обновите компилятор
   - Проверьте наличие всех зависимостей

### macOS

1. Ошибка "Qt not found":
   - Укажите путь к Qt: export Qt6_DIR=/usr/local/opt/qt@6
   - Добавьте путь в CMAKE_PREFIX_PATH

2. Проблемы с OpenSSL:
   - Укажите путь к OpenSSL в CMAKE_PREFIX_PATH
   - Проверьте версию OpenSSL

## Тестирование

```bash
# Сборка и запуск тестов
cd build
cmake -DBUILD_TESTS=ON ..
make
ctest
```

## Установка

### Windows
```bash
cmake --install . --prefix "C:/Program Files/Browser"
```

### Linux
```bash
sudo make install
```

### macOS
```bash
make install
```

## Создание пакетов

### Windows (NSIS installer)
```bash
cpack -G NSIS
```

### Linux (DEB)
```bash
cpack -G DEB
```

### Linux (RPM)
```bash
cpack -G RPM
```

### macOS (DMG)
```bash
cpack -G DragNDrop
```

## Дополнительная информация

- [Qt Documentation](https://doc.qt.io/)
- [CMake Documentation](https://cmake.org/documentation/)
- [WebEngine Documentation](https://doc.qt.io/qt-6/qtwebengine-index.html) 