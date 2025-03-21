QT += core gui serialport widgets printsupport

CONFIG += c++17
QMAKE_CXXFLAGS += -std=c++17

DESTDIR = bin
OBJECTS_DIR = bin/obj
MOC_DIR = bin/moc

INCLUDEPATH += include \
               lib \
               lib/qcustomplot

TARGET = uart-scope
TEMPLATE = app

SOURCES += \
    src/main.cpp \
    src/mainwindow.cpp \
    src/plotmanager.cpp \
    lib/qcustomplot/qcustomplot.cpp

HEADERS += \
    include/mainwindow.h \
    include/plotmanager.h \
    lib/qcustomplot/qcustomplot.h

QMAKE_POST_LINK += $$system(mkdir -p $$DESTDIR $$OBJECTS_DIR $$MOC_DIR)
