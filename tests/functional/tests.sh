#! /bin/sh

if test -z "$DBUS_SESSION_BUS_ADDRESS" ; then
	echo "No D-Bus session active; skipping functional tests"
	exit 0
fi

export QT_ACCESSIBILITY=1
export LC_ALL=C
export HOME="$SRCDIR/tests/functional"

"$SRCDIR/tests/functional/run-with-signon-ui.sh" \
	mago --nologcapture ./signon-ui-test.py
