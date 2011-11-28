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
    signon-plugins-common \
    libsignon-qt

HEADERS = \
    debug.h \
    dialog-request.h \
    dialog.h \
    i18n.h \
    request.h \
    service.h

SOURCES = \
    debug.cpp \
    dialog-request.cpp \
    dialog.cpp \
    i18n.cpp \
    main.cpp \
    request.cpp \
    service.cpp

DEFINES += DEBUG_ENABLED

