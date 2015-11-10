#include "previewhtml5.h"
#include "ui_previewhtml5.h"

PreviewHtml5::PreviewHtml5(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::PreviewHtml5)
{
    ui->setupUi(this);
    //url = ui->lineEdit->text();
    //ui->webView->load(QUrl(url));
}

PreviewHtml5::~PreviewHtml5()
{
    delete ui;
}

void PreviewHtml5::showMe()
{
    hide();
    show();
    url = ui->lineEdit->text();
    ui->webView->load(QUrl(url));
}
