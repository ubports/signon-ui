include(../common-project-config.pri)
include($${TOP_SRC_DIR}/common-vars.pri)
include($${TOP_SRC_DIR}/common-installs-config.pri)

TEMPLATE = app
TARGET = signon-ui

CONFIG += \
    link_pkgconfig \
    qt

QT += \
    core \
    dbus \
    gui

PKGCONFIG += \
    signon-plugins-common

HEADERS = \
    debug.h \
    request.h \
    signon-ui.h

SOURCES = \
    debug.cpp \
    main.cpp \
    request.cpp \
    signon-ui.cpp

DEFINES += DEBUG_ENABLED

