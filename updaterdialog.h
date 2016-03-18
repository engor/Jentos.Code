#ifndef UPDATERDIALOG_H
#define UPDATERDIALOG_H

#include <QDialog>

namespace Ui {
class UpdaterDialog;
}

class UpdaterDialog : public QDialog
{
    Q_OBJECT

public:
    explicit UpdaterDialog(QWidget *parent = 0);
    ~UpdaterDialog();

    void setParams(QString curVer, QString newVer, QString whatsNew);

private slots:
    void on_pushButton_clicked();

private:
    Ui::UpdaterDialog *ui;
};

#endif // UPDATERDIALOG_H
