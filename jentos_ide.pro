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
    findinfilesdialog.ui

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
    theme.cpp

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
    theme.h

RESOURCES += \
    resources.qrc \
    myresource.qrc

TARGET = JentosIDE

win32{
        RC_FILE = appicon.rc
}

mac{
#        WTF..enabling this appears to *break* 10.6 compatibility!!!!!
#        QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.6
        ICON = ted.icns
}
