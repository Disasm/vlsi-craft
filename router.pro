#-------------------------------------------------
#
# Project created by QtCreator 2015-03-21T14:46:00
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = router
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += \
    router/main.cpp \
    router/bordergrid.cpp \
    router/compactgrid.cpp \
    router/customgrid.cpp \
    router/gridstack.cpp \
    router/uniformgrid.cpp


HEADERS += \
    router/point.h \
    router/direction.h \
    router/bordergrid.h \
    router/compactgrid.h \
    router/customgrid.h \
    router/grid.h \
    router/gridstack.h \
    router/routerrules.h \
    router/uniformgrid.h

