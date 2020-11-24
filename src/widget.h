#ifndef WIDGET_H
#define WIDGET_H
//------------------------------------------------------------------------------
#include <QWidget>
#include <QSettings>
#include "filegenerator.h"
//------------------------------------------------------------------------------
namespace Ui {
class Widget;
}
class QFile;
class QThread;
class ProgressAndSpeedDialog;
//------------------------------------------------------------------------------
class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit        Widget(QWidget *parent = 0);
                   ~Widget();

signals:
    void            generateFile( QFile *file, int size, int speed );

public slots:
    void            generationFinished( FileGenerator::FinishReason r );

private slots:
    void on_fileNameCBX_toggled(bool checked);
    void on_fileNameLED_editingFinished();
    void on_sizeHSL_valueChanged(int value);
    void on_speedHSL_valueChanged(int value);
    void on_dirPBN_clicked();
    void on_startPBN_clicked();

protected:
    Ui::Widget      *ui;        // QWidget interface
    QSettings       settings;

    QString         fileName;
    QString         pathName;
    qint64          fileSizeMb;
    qint64          writeSpeedMb_s;

    QFile           *file;
    QThread         *thread;
    FileGenerator   *generator;

    void            closeEvent(QCloseEvent *event);
    QString         getTmpName();
    QString         getFullPathName();
};
//------------------------------------------------------------------------------
#endif // WIDGET_H