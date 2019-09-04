include(../nymea-gpio.pri)

TARGET = nymea-gpio-tool

CONFIG += console
CONFIG -= app_bundle
TEMPLATE = app

INCLUDEPATH += $$top_srcdir/libnymea-gpio/
LIBS += -L$$top_builddir/libnymea-gpio/ -lnymea-gpio

SOURCES += main.cpp \

target.path = /usr/bin
INSTALLS += target
