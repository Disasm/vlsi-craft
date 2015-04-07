#-------------------------------------------------
#
# Project created by QtCreator 2015-03-21T14:46:00
#
#-------------------------------------------------

QT       += core

QT       -= gui

TARGET = placer
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += \
    placer/main.cpp \
    common/netlist.cpp \
    placer/placementjob.cpp \
    placer/qp.cpp \
    placer/coomatrix.cpp \
    placer/array.cpp \
    placer/legalization.cpp \
    common/cellvariant.cpp \
    common/cellvariantlist.cpp \
    common/cellvariantfile.cpp


HEADERS += \
    common/netlist.h \
    placer/placementjob.h \
    placer/qp.h \
    placer/placement.h \
    common/vector.h \
    placer/coomatrix.h \
    placer/array.h \
    placer/legalization.h \
    common/cellvariant.h \
    common/cellvariantlist.h \
    common/cellvariantfile.h

