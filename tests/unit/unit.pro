include(../../common-project-config.pri)
include($${TOP_SRC_DIR}/common-vars.pri)
include($${TOP_SRC_DIR}/common-installs-config.pri)

TARGET = signon-ui-unittest

CONFIG += \
    build_all \
    debug \
    link_pkgconfig \
    qtestlib

QT += \
    core \
    dbus \
    gui \
    network \
    webkit

PKGCONFIG += \
    accounts-qt \
    signon-plugins-common \
    libsignon-qt

SOURCES += \
    fake-webcredentials-interface.cpp \
    test.cpp \
    $$TOP_SRC_DIR/src/browser-request.cpp \
    $$TOP_SRC_DIR/src/cookie-jar-manager.cpp \
    $$TOP_SRC_DIR/src/debug.cpp \
    $$TOP_SRC_DIR/src/dialog-request.cpp \
    $$TOP_SRC_DIR/src/dialog.cpp \
    $$TOP_SRC_DIR/src/network-access-manager.cpp \
    $$TOP_SRC_DIR/src/request.cpp
HEADERS += \
    fake-webcredentials-interface.h \
    test.h \
    $$TOP_SRC_DIR/src/browser-request.h \
    $$TOP_SRC_DIR/src/debug.h \
    $$TOP_SRC_DIR/src/cookie-jar-manager.h \
    $$TOP_SRC_DIR/src/dialog-request.h \
    $$TOP_SRC_DIR/src/dialog.h \
    $$TOP_SRC_DIR/src/network-access-manager.h \
    $$TOP_SRC_DIR/src/request.h
INCLUDEPATH += \
    . \
    $$TOP_SRC_DIR/src

QMAKE_CXXFLAGS += \
    -fno-exceptions \
    -fno-rtti

DEFINES += \
    DEBUG_ENABLED \
    UNIT_TESTS

check.commands = "./signon-ui-unittest"
QMAKE_EXTRA_TARGETS += check

