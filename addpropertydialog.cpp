#include "addpropertydialog.h"
#include "ui_addpropertydialog.h"

AddPropertyDialog::AddPropertyDialog(QWidget *parent, Qt::WindowFlags f) :
    QDialog(parent),
    ui(new Ui::AddPropertyDialog)
{
    ui->setupUi(this);
    pressOk = false;
}

AddPropertyDialog::~AddPropertyDialog()
{
    delete ui;
}

void AddPropertyDialog::fillTypes(QStringList &list)
{
    ui->comboBoxPropType->clear();
    ui->comboBoxPropType->addItems(list);
}

void AddPropertyDialog::on_pushButtonCancel_clicked()
{
    hide();
}

void AddPropertyDialog::on_pushButtonAdd_clicked()
{
    propName = ui->lineEditPropName->text().trimmed();
    if (propName.isEmpty()) {
        QMessageBox::information(this, "Invalid Value", "Invalid 'Property Name' value.");
        ui->lineEditPropName->setFocus();
        return;
    }
    propType = ui->comboBoxPropType->currentText().trimmed();
    if (propType.isEmpty()) {
        QMessageBox::information(this, "Invalid Value", "Invalid 'Property Type' value.");
        ui->comboBoxPropType->setFocus();
        return;
    }
    wrappedName = ui->lineEditWrappedName->text().trimmed();
    if (wrappedName.isEmpty()) {
        QMessageBox::information(this, "Invalid Value", "Invalid 'Wrapped field Name' value.");
        ui->lineEditPropName->setFocus();
        return;
    }
    if (wrappedName == propName) {
        QMessageBox::information(this, "Invalid Value", "'Property Name' value must be different from 'Wrapped field Name' value.");
        ui->lineEditPropName->setFocus();
        return;
    }
    addVariant = ui->comboBoxAddVariant->currentIndex();
    isNeedToAddWrapped = ui->checkBoxAddWrapped->isChecked();
    pressOk = true;
    hide();
}

void AddPropertyDialog::on_lineEditPropName_textChanged(const QString &arg1)
{
    if (!ui->checkBoxAutoNameWrapped->isChecked())
        return;
    if (arg1.isEmpty())
        return;
    QString s = "_" + QString(arg1.at(0).toLower()) + arg1.mid(1);
    ui->lineEditWrappedName->setText(s);
}
