include(../common-project-config.pri)
include($${TOP_SRC_DIR}/common-vars.pri)
include($${TOP_SRC_DIR}/common-installs-config.pri)

TEMPLATE = app
TARGET = signon-ui-test

CONFIG += \
    qt \
    qtestlib

QT += \
    core

HEADERS = \
    signon-ui-test.h

SOURCES = \
    signon-ui-test.cpp

INCLUDEPATH += \
    $${TOP_SRC_DIR}/src

QMAKE_LIBDIR += \
    $${TOP_BUILD_DIR}/src
QMAKE_RPATHDIR = $${QMAKE_LIBDIR}

check.commands = "./signon-ui-test -v1"
check.depends = signon-ui-test
QMAKE_EXTRA_TARGETS += check

