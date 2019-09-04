TARGET = nymea-gpio
TEMPLATE = lib

QT -= gui

QMAKE_CXXFLAGS *= -Werror -std=c++11 -g
QMAKE_LFLAGS *= -std=c++11

VERSION_STRING=$$system('dpkg-parsechangelog | sed -n -e "s/^Version: //p"')

DEFINES += LIBNYMEAGPIO_LIBRARY

SOURCES += \
        gpio.cpp \
        gpiomonitor.cpp

HEADERS += \
        gpio.h \
        gpiomonitor.h

target.path = $$[QT_INSTALL_LIBS]
INSTALLS += target

# install header file with relative subdirectory
for(header, HEADERS) {
    path = /usr/include/libnymea-gpio/$${dirname(header)}
    eval(headers_$${path}.files += $${header})
    eval(headers_$${path}.path = $${path})
    eval(INSTALLS *= headers_$${path})
}

# Create pkgconfig file
CONFIG += create_pc create_prl no_install_prl
QMAKE_PKGCONFIG_NAME = libnymea-gpio
QMAKE_PKGCONFIG_DESCRIPTION = nymea gpio development library
QMAKE_PKGCONFIG_PREFIX = $$[QT_INSTALL_PREFIX]
QMAKE_PKGCONFIG_INCDIR = $$[QT_INSTALL_PREFIX]/include/libnymea-gpio/
QMAKE_PKGCONFIG_LIBDIR = $$target.path
QMAKE_PKGCONFIG_VERSION = $$VERSION_STRING
QMAKE_PKGCONFIG_FILE = libnymea-gpio
QMAKE_PKGCONFIG_DESTDIR = pkgconfig