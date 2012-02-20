include(../../common-project-config.pri)
include($${TOP_SRC_DIR}/common-vars.pri)
include($${TOP_SRC_DIR}/common-installs-config.pri)

TEMPLATE = subdirs

RUN_WITH_SIGNON_UI = "BUILDDIR=$$TOP_BUILD_DIR HOME=$$TOP_SRC_DIR/tests/functional/ $$TOP_SRC_DIR/tests/functional/run-with-signon-ui.sh"

check.commands = "QT_ACCESSIBILITY=1 LC_ALL=C $$RUN_WITH_SIGNON_UI mago --nologcapture ./signon-ui-test.py"
QMAKE_EXTRA_TARGETS += check

DISTFILES += \
    .config/signon-ui/webkit-options.d/localhost.conf
