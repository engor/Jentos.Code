
#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "prefs.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTime t;
    int msec = t.msec();

    QSplashScreen *splash = new QSplashScreen( QPixmap(":ui/splash.png") );
    splash->show();

    MainWindow w;

    int dt = t.msec()-(msec+2000);
    if (dt > 0)
        QThread::msleep(dt);

    w.show();
    splash->finish(&w);
    delete splash;

    if (!Prefs::prefs()->isValidMonkeyPath) {
        QMessageBox::warning( &w, "Invalid Path", "Invalid Monkey path!\n\nPlease select correct path from the 'File -- Options' dialog.", "Go to Options" );
        w.onFilePrefs(true);
    }

    return a.exec();
}
