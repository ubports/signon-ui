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
    libnotify \
    libsignon-qt

SOURCES += \
    fake-libnotify.cpp \
    fake-libsignon.cpp \
    fake-webcredentials-interface.cpp \
    test.cpp \
    $$TOP_SRC_DIR/src/animation-label.cpp \
    $$TOP_SRC_DIR/src/browser-request.cpp \
    $$TOP_SRC_DIR/src/cookie-jar-manager.cpp \
    $$TOP_SRC_DIR/src/debug.cpp \
    $$TOP_SRC_DIR/src/dialog-request.cpp \
    $$TOP_SRC_DIR/src/dialog.cpp \
    $$TOP_SRC_DIR/src/i18n.cpp \
    $$TOP_SRC_DIR/src/indicator-service.cpp \
    $$TOP_SRC_DIR/src/network-access-manager.cpp \
    $$TOP_SRC_DIR/src/reauthenticator.cpp \
    $$TOP_SRC_DIR/src/request.cpp \
    $$TOP_SRC_DIR/src/webcredentials_adaptor.cpp
HEADERS += \
    fake-libnotify.h \
    fake-webcredentials-interface.h \
    test.h \
    $$TOP_SRC_DIR/src/animation-label.h \
    $$TOP_SRC_DIR/src/browser-request.h \
    $$TOP_SRC_DIR/src/debug.h \
    $$TOP_SRC_DIR/src/cookie-jar-manager.h \
    $$TOP_SRC_DIR/src/dialog-request.h \
    $$TOP_SRC_DIR/src/dialog.h \
    $$TOP_SRC_DIR/src/indicator-service.h \
    $$TOP_SRC_DIR/src/network-access-manager.h \
    $$TOP_SRC_DIR/src/reauthenticator.h \
    $$TOP_SRC_DIR/src/request.h \
    $$TOP_SRC_DIR/src/webcredentials_adaptor.h

lessThan(QT_MAJOR_VERSION, 5) {
    SOURCES += $$TOP_SRC_DIR/src/embed-manager.cpp
    HEADERS += $$TOP_SRC_DIR/src/embed-manager.h
}

INCLUDEPATH += \
    . \
    $$TOP_SRC_DIR/src

QMAKE_CXXFLAGS += \
    -fno-exceptions \
    -fno-rtti

DEFINES += \
    DEBUG_ENABLED \
    UNIT_TESTS

RESOURCES += $$TOP_SRC_DIR/src/animationlabel.qrc

check.commands = "xvfb-run dbus-test-runner -t ./signon-ui-unittest"
QMAKE_EXTRA_TARGETS += check
