#! /bin/sh

if test -z "$DISPLAY" ; then
	echo "No X11 display; skipping functional tests"
	exit 0
fi

export LC_ALL=C
export HOME="$SRCDIR/tests/functional"
export SSOUI_DAEMON_TIMEOUT=10

"$SRCDIR/tests/functional/run-with-signon-ui.sh" \
	./dialog.rb

# Web tests
./server.rb &
SERVER_PID="$!"
"$SRCDIR/tests/functional/run-with-signon-ui.sh" \
	./webpage.rb
kill "$SERVER_PID"

