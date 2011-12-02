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
    gui \
    webkit

PKGCONFIG += \
    signon-plugins-common \
    libsignon-qt

HEADERS = \
    browser-request.h \
    debug.h \
    dialog-request.h \
    dialog.h \
    i18n.h \
    request.h \
    service.h

SOURCES = \
    browser-request.cpp \
    debug.cpp \
    dialog-request.cpp \
    dialog.cpp \
    i18n.cpp \
    main.cpp \
    request.cpp \
    service.cpp

DEFINES += DEBUG_ENABLED

service.path = $${INSTALL_PREFIX}/share/dbus-1/services
service.files = com.nokia.singlesignonui.service
INSTALLS += service
