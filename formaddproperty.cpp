#include "formaddproperty.h"
#include "ui_formaddproperty.h"

FormAddProperty::FormAddProperty(QWidget *parent, Qt::WindowFlags f) :
    QDialog(parent),
    ui(new Ui::FormAddProperty)
{
    ui->setupUi(this);
    pressOk = false;
}

FormAddProperty::~FormAddProperty()
{
    delete ui;
}

void FormAddProperty::fillTypes(QStringList &list)
{
    ui->comboBoxPropType->clear();
    ui->comboBoxPropType->addItems(list);
}

void FormAddProperty::on_pushButtonCancel_clicked()
{
    hide();
}

void FormAddProperty::on_pushButtonAdd_clicked()
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

void FormAddProperty::on_lineEditPropName_textChanged(const QString &arg1)
{
    if (!ui->checkBoxAutoNameWrapped->isChecked())
        return;
    if (arg1.isEmpty())
        return;
    QString s = "_" + QString(arg1.at(0).toLower()) + arg1.mid(1);
    ui->lineEditWrappedName->setText(s);
}
