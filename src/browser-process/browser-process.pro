include(../../common-project-config.pri)
include($${TOP_SRC_DIR}/common-vars.pri)

TEMPLATE = app
TARGET = browser-process

target.path = $${INSTALL_PREFIX}/libexec/signon-ui
INSTALLS += target

I18N_DOMAIN = signon-ui

CONFIG += \
    link_pkgconfig \
    qt

QT += \
    core \
    gui \
    quick

PKGCONFIG += \
    signon-plugins-common

INCLUDEPATH += ..

HEADERS = \
    browser-process.h \
    debug.h \
    dialog.h \
    ../i18n.h \
    ../remote-request-interface.h
SOURCES = \
    browser-process.cpp \
    dialog.cpp \
    main.cpp \
    ../i18n.cpp \
    ../remote-request-interface.cpp

DEFINES += \
    DEBUG_ENABLED \
    I18N_DOMAIN=\\\"$${I18N_DOMAIN}\\\"

CONFIG(force-foreign-qwindow) {
    DEFINES += FORCE_FOREIGN_QWINDOW
}

OTHER_FILES += \
    webview.qml \
    KeyboardRectangle.qml \
    StandardAnimation.qml

RESOURCES += \
    qml.qrc

QMAKE_SUBSTITUTES += \
    signon-ui-browser-process.desktop.in
desktop.path = $${INSTALL_PREFIX}/share/applications
desktop.files += signon-ui-browser-process.desktop
INSTALLS += desktop

