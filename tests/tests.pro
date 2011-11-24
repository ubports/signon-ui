include(../common-project-config.pri)
include($${TOP_SRC_DIR}/common-vars.pri)
include($${TOP_SRC_DIR}/common-installs-config.pri)

TEMPLATE = app
TARGET = signon-ui-test

CONFIG += \
    link_pkgconfig \
    qt \
    qtestlib

QT += \
    core \
    dbus

PKGCONFIG += \
    signon-plugins-common

HEADERS = \
    signon-ui-test.h

SOURCES = \
    signon-ui-test.cpp

INCLUDEPATH += \
    $${TOP_SRC_DIR}/src

QMAKE_LIBDIR += \
    $${TOP_BUILD_DIR}/src
QMAKE_RPATHDIR = $${QMAKE_LIBDIR}

RUN_WITH_SIGNON_UI = "BUILDDIR=$$TOP_BUILD_DIR $$TOP_SRC_DIR/tests/run-with-signon-ui.sh"

check.commands = "$$RUN_WITH_SIGNON_UI ./signon-ui-test -v1"
check.depends = signon-ui-test
QMAKE_EXTRA_TARGETS += check

