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
    debug.h

SOURCES = \
    main.cpp

