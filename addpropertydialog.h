#ifndef ADDPROPERTYDIALOG_H
#define ADDPROPERTYDIALOG_H

#include <QDialog>
#include <QMessageBox>


namespace Ui {
class AddPropertyDialog;
}

class AddPropertyDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddPropertyDialog(QWidget *parent = 0, Qt::WindowFlags f = 0);
    ~AddPropertyDialog();

    static const int ADD_GETTER_THEN_SETTER = 0;
    static const int ADD_SETTER_THEN_GETTER = 1;
    static const int ADD_GETTER_ONLY = 2;
    static const int ADD_SETTER_ONLY = 3;

    void fillTypes(QStringList &list);

    QString propName, propType, wrappedName;
    bool isNeedToAddWrapped;
    bool pressOk;
    int addVariant;

private slots:
    void on_pushButtonCancel_clicked();

    void on_pushButtonAdd_clicked();

    void on_lineEditPropName_textChanged(const QString &arg1);

private:
    Ui::AddPropertyDialog *ui;
};

#endif // FORMADDPROPERTY_H
