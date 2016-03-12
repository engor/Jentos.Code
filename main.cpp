
#include "mainwindow.h"
#include "ui_mainwindow.h"

int main(int argc, char *argv[]) {
    QApplication application(argc, argv);
    MainWindow window;
    window.show();
    return application.exec();
}
