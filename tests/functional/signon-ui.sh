#! /bin/sh

set -e

export SSOUI_LOGGING_LEVEL=2
${SSOUI_WRAPPER} ${BUILDDIR}/src/signon-ui
