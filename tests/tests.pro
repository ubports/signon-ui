TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = \
    unit

CONFIG(medium-tests) {
    SUBDIRS += functional
}
