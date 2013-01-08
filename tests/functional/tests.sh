#! /bin/sh

if test -z "$DISPLAY" ; then
	echo "No X11 display; skipping functional tests"
	exit 0
fi

export LC_ALL=C
export HOME="$SRCDIR/tests/functional"
export SSOUI_DAEMON_TIMEOUT=10

"$SRCDIR/tests/functional/run-with-signon-ui.sh" \
	./reauthenticator.py
