QT += core gui network sql widgets webenginecore webenginewidgets webengine webchannel

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/adblocker.cpp \
    src/bookmarkmanager.cpp \
    src/extensionmanager.cpp \
    src/historymanager.cpp \
    src/syncmanager.cpp \
    src/tabwidget.cpp \
    src/webview.cpp \
    src/downloadmanager.cpp \
    src/settings.cpp \
    src/readermode.cpp

HEADERS += \
    src/mainwindow.h \
    src/adblocker.h \
    src/bookmarkmanager.h \
    src/extensionmanager.h \
    src/historymanager.h \
    src/syncmanager.h \
    src/tabwidget.h \
    src/webview.h \
    src/downloadmanager.h \
    src/settings.h \
    src/readermode.h

FORMS += \
    src/mainwindow.ui \
    src/bookmarkdialog.ui \
    src/downloadmanager.ui \
    src/settings.ui

RESOURCES += \
    resources.qrc

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

# Include paths
INCLUDEPATH += \
    src \
    $$[QT_INSTALL_HEADERS]/QtWebEngine \
    $$[QT_INSTALL_HEADERS]/QtWebEngineCore \
    $$[QT_INSTALL_HEADERS]/QtWebEngineWidgets \
    $$[QT_INSTALL_HEADERS]/QtWebChannel

# Windows specific settings
win32 {
    RC_ICONS = resources/icons/brouser.ico
    VERSION = 1.0.0.0
    QMAKE_TARGET_COMPANY = "Brouser Project"
    QMAKE_TARGET_PRODUCT = "Brouser"
    QMAKE_TARGET_DESCRIPTION = "Modern Web Browser"
    QMAKE_TARGET_COPYRIGHT = "Copyright (C) 2024"
    
    # Windows specific libraries
    LIBS += -luser32 -lshell32
}

# macOS specific settings
macx {
    ICON = resources/icons/brouser.icns
    QMAKE_INFO_PLIST = resources/Info.plist
}

# Linux specific settings
unix:!macx {
    desktop.path = /usr/share/applications
    desktop.files += resources/linux/brouser.desktop
    icon.path = /usr/share/icons/hicolor/256x256/apps
    icon.files += resources/icons/brouser.png
    INSTALLS += desktop icon
}

# Additional compiler warnings
*g++* {
    QMAKE_CXXFLAGS += -Wall -Wextra -Wpedantic
}

*msvc* {
    QMAKE_CXXFLAGS += /W4
}

# Define debug/release specific options
CONFIG(debug, debug|release) {
    DEFINES += DEBUG
    win32: CONFIG += console
}

CONFIG(release, debug|release) {
    DEFINES += QT_NO_DEBUG_OUTPUT
    DEFINES += QT_NO_WARNING_OUTPUT
    DEFINES += NDEBUG
}

# Ensure the target name is set
TARGET = brouser 