include(../../common-project-config.pri)
include($${TOP_SRC_DIR}/common-vars.pri)
include($${TOP_SRC_DIR}/common-installs-config.pri)

TEMPLATE = subdirs

check.commands = "BUILDDIR=$$TOP_BUILD_DIR SRCDIR=$$TOP_SRC_DIR $$TOP_SRC_DIR/tests/functional/tests.sh"
QMAKE_EXTRA_TARGETS += check
