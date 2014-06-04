include(../../common-project-config.pri)
include($${TOP_SRC_DIR}/common-vars.pri)

TARGET = tst_inactivity_timer

CONFIG += \
    build_all \
    debug \
    qtestlib

QT += \
    core

SOURCES += \
    tst_inactivity_timer.cpp \
    $$TOP_SRC_DIR/src/debug.cpp \
    $$TOP_SRC_DIR/src/inactivity-timer.cpp
HEADERS += \
    $$TOP_SRC_DIR/src/debug.h \
    $$TOP_SRC_DIR/src/inactivity-timer.h

INCLUDEPATH += \
    . \
    $$TOP_SRC_DIR/src

QMAKE_CXXFLAGS += \
    -fno-exceptions \
    -fno-rtti

DEFINES += \
    DEBUG_ENABLED \
    UNIT_TESTS

check.commands = "xvfb-run -a ./$$TARGET"
check.depends = $$TARGET
QMAKE_EXTRA_TARGETS += check
