QT += widgets testlib
CONFIG += c++17
TARGET = demo
INCLUDEPATH += ..
SOURCES += demo.cpp ../editorwindow.cpp ../codeeditor.cpp
HEADERS += ../editorwindow.h ../codeeditor.h
FORMS += ../editorwindow.ui
