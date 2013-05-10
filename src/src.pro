include(../common-project-config.pri)

TEMPLATE = subdirs
SUBDIRS = signon-ui.pro

CONFIG(use-webkit2) {
    lessThan(QT_MAJOR_VERSION, 5) {
        error('use-webkit2' option is supported only with Qt5)
    }
    SUBDIRS += browser-process
}
