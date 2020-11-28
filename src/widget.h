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
    explicit                    Widget(QWidget *parent = 0);
                               ~Widget();

signals:
    void                        generateFile( QFile                         *file,
                                              int                           size,
                                              int                           speed,
                                              int                           sinNum );

public slots:
    void                        generationFinished( FileGenerator::FinishReason r );

private slots:
    void                        on_fileNameCBX_toggled(bool checked);
    void                        on_fileNameLED_editingFinished();
    void                        on_sizeHSL_valueChanged(int value);
    void                        on_speedHSL_valueChanged(int value);
    void                        on_dirPBN_clicked();
    void                        on_startPBN_clicked();

    void on_sinHSL_valueChanged(int value);

private:
    enum RealizeType{ Noise, NoiseAndSin };
protected:
    Ui::Widget                  *ui;        // QWidget interface
    QSettings                   settings;

    QString                     fileName;       // имя файла
    QString                     pathName;       // путь до файла
    qint64                      fileSizeMb;     // сколько нужно мегабайт
    qint64                      writeSpeedMb_s; // требуемая максимальная скорость

    RealizeType                 realizeType;    // тип формируемой реализации
    int                         sinNumber;      // число гармоник

    QFile                       *file;          // файл
    QThread                     *thread;        // поток, в котором выполняется генератор
    FileGenerator               *generator;     // генератор файла

    void                        closeEvent(QCloseEvent *event);
    QString                     getTmpName();
    QString                     getFullPathName();
};
//------------------------------------------------------------------------------
#endif // WIDGET_H
