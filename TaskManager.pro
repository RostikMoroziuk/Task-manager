#-------------------------------------------------
#
# Project created by QtCreator 2017-03-16T07:45:05
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = TaskManager
TEMPLATE = app

#для коректної компіляції з winAPI
win32:LIBS += -luser32
win32:LIBS += -lpsapi
win32:LIBS += -lkernel32

SOURCES += main.cpp\
        taskmanager.cpp

HEADERS  += taskmanager.h

FORMS    += taskmanager.ui
