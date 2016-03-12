#-------------------------------------------------
#
# Project created by QtCreator 2012-04-28T10:44:05
#
#-------------------------------------------------

QT       += core gui webkit widgets webkitwidgets

TEMPLATE = app

FORMS    += mainwindow.ui \
    finddialog.ui \
    prefsdialog.ui \
    findinfilesdialog.ui \
    previewhtml5.ui \
    formaddproperty.ui

SOURCES += main.cpp\
        mainwindow.cpp \
    codeeditor.cpp \
    colorswatch.cpp \
    projecttreemodel.cpp \
    std.cpp \
    debugtreemodel.cpp \
    finddialog.cpp \
    prefs.cpp \
    prefsdialog.cpp \
    findinfilesdialog.cpp \
    quickhelp.cpp \
    listwidgetcomplete.cpp \
    codeanalyzer.cpp \
    tabwidgetdrop.cpp \
    proc.cpp \
    theme.cpp \
    previewhtml5.cpp \
    customcombobox.cpp \
    formaddproperty.cpp

HEADERS  += mainwindow.h \
    codeeditor.h \
    colorswatch.h \
    projecttreemodel.h \
    std.h \
    debugtreemodel.h \
    finddialog.h \
    prefs.h \
    prefsdialog.h \
    findinfilesdialog.h \
    quickhelp.h \
    listwidgetcomplete.h \
    codeanalyzer.h \
    tabwidgetdrop.h \
    proc.h \
    theme.h \
    previewhtml5.h \
    customcombobox.h \
    formaddproperty.h

RESOURCES += \
    resources.qrc
#    resources.qrc

TARGET = Jentos.Code
#OK, this seems to prevent latest Windows QtCreator from being able to run Ted (builds fine).
#Solved by using qtcreator-2.4.1
DESTDIR = exec

win32{
        RC_FILE = appicon.rc
}

mac{
#        WTF..enabling this appears to *break* 10.6 compatibility!!!!!
#        QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.6
        ICON = ted.icns
}
