TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += main.c \
    connectserver.c

HEADERS += \
    connectserver.h
LIBS += \
 -lpthread
