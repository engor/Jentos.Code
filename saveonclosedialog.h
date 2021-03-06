#ifndef SAVEONCLOSEDIALOG_H
#define SAVEONCLOSEDIALOG_H

#include <QDialog>

namespace Ui {
class SaveOnCloseDialog;
}

class SaveOnCloseDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SaveOnCloseDialog(QWidget *parent = 0);
    ~SaveOnCloseDialog();

    void fillList(QStringList &list);
    QStringList filesToSave() {return list;}

private slots:
    void on_pushButtonSave_clicked();

    void on_pushButtonDiscard_clicked();

    void on_pushButtonCancel_clicked();

private:
    Ui::SaveOnCloseDialog *ui;
    QStringList list;
};

#endif // SAVEONCLOSEDIALOG_H
