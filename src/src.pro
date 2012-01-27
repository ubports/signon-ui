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
    network \
    webkit

PKGCONFIG += \
    signon-plugins-common \
    libsignon-qt

HEADERS = \
    browser-request.h \
    cookie-jar-manager.h \
    debug.h \
    dialog-request.h \
    dialog.h \
    errors.h \
    i18n.h \
    network-access-manager.h \
    request.h \
    service.h

SOURCES = \
    browser-request.cpp \
    cookie-jar-manager.cpp \
    debug.cpp \
    dialog-request.cpp \
    dialog.cpp \
    i18n.cpp \
    main.cpp \
    network-access-manager.cpp \
    request.cpp \
    service.cpp

DEFINES += DEBUG_ENABLED

service.path = $${INSTALL_PREFIX}/share/dbus-1/services
service.files = com.nokia.singlesignonui.service
INSTALLS += service
