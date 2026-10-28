QT += widgets testlib
CONFIG += c++17 testcase
TARGET = test_editor
INCLUDEPATH += ..
SOURCES += test_editor.cpp ../editorwindow.cpp ../codeeditor.cpp
HEADERS += ../editorwindow.h ../codeeditor.h
FORMS += ../editorwindow.ui
