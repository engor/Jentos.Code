#ifndef PREVIEWHTML5_H
#define PREVIEWHTML5_H

#include <QMainWindow>

namespace Ui {
class PreviewHtml5;
}

class PreviewHtml5 : public QMainWindow
{
    Q_OBJECT

public:
    explicit PreviewHtml5(QWidget *parent = 0);
    ~PreviewHtml5();
    void showMe();

private:
    Ui::PreviewHtml5 *ui;
    QString url;

};

#endif // PREVIEWHTML5_H
