#! /bin/sh

set -e

qttasserver &

# start a local signon-ui
dbus-test-runner -m 180 \
	-t ${SRCDIR}/tests/functional/signon-ui.sh \
	-t "$@" -f com.nokia.singlesignonui
