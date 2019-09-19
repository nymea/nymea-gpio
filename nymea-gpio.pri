
QMAKE_CXXFLAGS += -Werror -std=c++11 -g
QMAKE_LFLAGS += -std=c++11

QT -= gui

top_srcdir=$$PWD
top_builddir=$$shadowed($$PWD)

VERSION_STRING=$$system('dpkg-parsechangelog | sed -n -e "s/^Version: //p"')
DEFINES += VERSION_STRING=\\\"$${VERSION_STRING}\\\"
