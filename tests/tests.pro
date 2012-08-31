TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = \
    unit

# Functional tests are disabled, because of some tdriver/ruby issues
# SUBDIRS += functional
