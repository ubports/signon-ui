#! /bin/sh

# If there's already an instance of signon-ui running, kill it
pkill signon-ui

set -e

trap "pkill -9 signon-ui" EXIT

# start a local signon-ui

export SSOUI_LOGGING_LEVEL=2
${BUILDDIR}/src/signon-ui &
sleep 2

${CLIENT_WRAPPER} $@

