#-------------------------------------------------
#
# Project created by QtCreator 2012-04-28T10:44:05
#
#-------------------------------------------------

QT       += core gui widgets webenginewidgets

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
    theme.cpp \
    consolelog.cpp

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
    consolelog.h

RESOURCES += \
    resources.qrc \
    myresource.qrc

TARGET = JentosIDE

win32{
        RC_FILE = jentos.rc
}

mac{
#        WTF..enabling this appears to *break* 10.6 compatibility!!!!!
#        QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.6
        ICON = jentos.icns
}
