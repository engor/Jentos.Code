#include "updaterdialog.h"
#include "ui_updaterdialog.h"

UpdaterDialog::UpdaterDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::UpdaterDialog)
{
    ui->setupUi(this);
}

UpdaterDialog::~UpdaterDialog()
{
    delete ui;
}

void UpdaterDialog::setParams(QString curVer, QString newVer, QString whatsNew)
{
    ui->labelCurrent->setText("Current version: "+curVer);
    ui->labelNew->setText("New version: "+newVer);
    //ui->plainTextEdit->insertPlainText();
    ui->textBrowser->setText(whatsNew);
    show();
}

void UpdaterDialog::on_pushButton_clicked()
{
    hide();
}
