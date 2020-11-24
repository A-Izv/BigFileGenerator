#include "widget.h"
#include "ui_widget.h"
//------------------------------------------------------------------------------
#include <QFile>
#include <QDir>
#include <QTime>
#include <QDebug>
#include <QThread>
#include <QMessageBox>
#include <QFileDialog>
//------------------------------------------------------------------------------
#include "appGlobal.h"
#include "progressandspeeddialog.h"
//------------------------------------------------------------------------------
Widget::Widget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::Widget),
    settings("theCompany","BigFileGenerator"),
    file(nullptr),
    thread(nullptr),
    generator(nullptr)
{
    ui->setupUi(this);
 // настройки главного окна (this-> для наглядности)
    // фиксируем размер окна по вертикали
    this->setMaximumHeight( this->layout()->minimumSize().height() );

 // инициализация генератора случайных чисел
    qsrand( QTime::currentTime().msecsSinceStartOfDay() );

 // класс генерации
    thread      = new QThread(this);
    generator   = new FileGenerator();  // у объекта, перемещаемого в другой поток
                                        // не должно быть родителя
    // СОЕДИНЕНИЯ
    // соединяем сигнал с запросом на генерацию со слотом генератора
    QObject::connect( this,      &Widget::generateFile,
                      generator, &FileGenerator::generateFile );
    // сигнал о завершении генерации с локальным слотом
    QObject::connect( generator, &FileGenerator::generationFinished,
                      this,      &Widget::generationFinished );

    // перенос объекта в другой поток - теперь все слоты будут выполнятся в этом потоке
    generator->moveToThread( thread );  // переносим
    thread->start();                    // запускаем поток - иначе обработка не начнется
 // восстановление настроек пользовательского интерфейса
    // местоположение
    restoreGeometry( settings.value( "geometry" ).toByteArray() );
    // путь
    pathName = settings.value( "pathName", QDir::homePath() ).toString();
    ui->dirLED->setText( QDir::toNativeSeparators(pathName) );
    // имя файла
    ui->fileNameCBX->setChecked( settings.value("randomName",true).toBool() );
    if( ui->fileNameCBX->isChecked() ) {
        // генерируем случайное имя
        on_fileNameCBX_toggled( true );
    } else {
        // восстанавливаем имя из настроек
        fileName = settings.value( "fileName", "file.pcm" ).toString();
        ui->fileNameLED->setText( fileName );
    }
    // размер
    fileSizeMb = settings.value( "fileSizeMb", 60 ).toInt();
    ui->sizeHSL->setValue( fileSizeMb );
    // скорость записи
    ui->speedCBX->setChecked( settings.value("maxSpeed",true).toBool() );
    writeSpeedMb_s = settings.value( "writeSpeedMb_s", 2 ).toInt();
    ui->speedHSL->setValue( writeSpeedMb_s );
}
//------------------------------------------------------------------------------
Widget::~Widget()
{
 // сохранение параметров пользовательского инетерфейса
    settings.setValue( "geometry",          saveGeometry() );
    settings.setValue( "pathName",          pathName );
    settings.setValue( "randomName",        ui->fileNameCBX->isChecked() );
    settings.setValue( "fileName",          fileName );
    settings.setValue( "fileSizeMb",        fileSizeMb );
    settings.setValue( "maxSpeed",          ui->speedCBX->isChecked() );
    settings.setValue( "writeSpeedMb_s",    writeSpeedMb_s );

 // удаляем объекты
    thread->quit();
    thread->wait();
    delete  thread;     thread      = nullptr;

    delete generator;   generator   = nullptr;

    delete file;        file        = nullptr;

    delete ui;
}
//------------------------------------------------------------------------------
void Widget::closeEvent(QCloseEvent *event)
{// главное окно приложения закрывают
    generator->abortGeneration();
    Global::TerminateFlag = true; // выставляем глобальный флаг - чтобы завершить текущую обработку
    QWidget::closeEvent(event); // обрабатываем событие по умолчанию
}
//------------------------------------------------------------------------------
QString Widget::getTmpName()
{
    static const int    RANDOM_COUNT = 3;
    QByteArray          b;
    QString             result;
    quint16             tmpU;

    // набиваем QByteArray случайными данными
    for( int i = 0 ; i < RANDOM_COUNT ; ++i ) {
        tmpU = qrand();
        b.append( (char*)(&tmpU), sizeof(tmpU) );
    }

    // формируем имя файла
    result  = "TMP.";                                       // префикс
    result += b.toBase64( QByteArray::Base64UrlEncoding |   // случайная часть
                          QByteArray::OmitTrailingEquals );
    result.append( ".pcm" );                                // суффикс

    return result;
}
//------------------------------------------------------------------------------
void Widget::on_fileNameCBX_toggled(bool checked)
{
    if( checked ) {
        fileName = getTmpName();
        ui->fileNameLED->setText( fileName );   // перенос в пользовательский интерфейс
    }
}
//------------------------------------------------------------------------------
void Widget::on_fileNameLED_editingFinished()
{// окончилось ручное редактирование имени файла - запоминаем новое имя
    fileName = ui->fileNameLED->text();
}
//------------------------------------------------------------------------------
void Widget::on_sizeHSL_valueChanged(int value)
{
    fileSizeMb = value;         // запоминаем новое значение размера
}
//------------------------------------------------------------------------------
void Widget::on_speedHSL_valueChanged(int value)
{
    writeSpeedMb_s = value;     // запоминаем новое значение скорости
}
//------------------------------------------------------------------------------
void Widget::on_dirPBN_clicked()
{
    QString s = QFileDialog::getExistingDirectory(
                    this,
                    "Выберите каталог для создания файла",
                    pathName
                );

    if( !s.isEmpty() ) { // если был выбран новый каталог - запоминаем его
        pathName = s;
        ui->dirLED->setText( QDir::toNativeSeparators(pathName) );
    }
}
//------------------------------------------------------------------------------
QString Widget::getFullPathName()
{
    QString result;
    bool    flag = true;

    result = pathName + QDir::separator() + fileName;
    result = QDir::fromNativeSeparators(result);

 // проверяем, существует ли файл
    while( flag && QFile::exists(result) ) {
     // файл существует, создаем диалог с кастомными кнопками
        QMessageBox *box = new QMessageBox( this );

        box->setIcon( QMessageBox::Warning );
        box->setWindowTitle( "Файл уже существует" );
        box->addButton( QMessageBox::Ok     )->setText( "Перезаписать" );
        box->addButton( QMessageBox::Cancel )->setText( "Отменить"     );
        if( ui->fileNameCBX->isChecked() ) {
         // если имя случайное - появляется возможность создать файл с другим случайным именем
            box->addButton( QMessageBox::Reset )->setText( "С новым именем" );
            box->setText( "Файл уже существует.\n"
                          "Можно отменить обработку, "
                          "перезаписать файл или "
                          "сформировать файл с новым именем." );
        } else {
            box->setText( "Файл уже существует.\n"
                          "Можно отменить обработку или "
                          "перезаписать файл." );
        }
        box->setDefaultButton( QMessageBox::Cancel ); // по умолчанию - отмена

        int dialogResult = box->exec();
        switch( dialogResult ) {
         // файл будет перезаписан
            case QMessageBox::Ok :
                    flag = false;
                    break;
         // новое имя
            case QMessageBox::Reset :
                    fileName = getTmpName();
                    ui->fileNameLED->setText( fileName );

                    result = pathName + QDir::separator() + fileName;
                    result = QDir::fromNativeSeparators(result);
                    // флаг не сбрасываем, чтобы еще раз проверить наличие файла
                    break;
         // любой другой исход - отмена
            default :
                    result.clear();
                    flag = false;
        }
        delete box;
    }
    return result;
}
//------------------------------------------------------------------------------
void Widget::on_startPBN_clicked()
{
    QString fullName = getFullPathName();

 // создаем объект для доступа к файлу
    if( !fullName.isEmpty() ) {
        file = new QFile( fullName );

        try {
         // открываем файл
            if( !file->open(QIODevice::WriteOnly) ) {
                throw("error: file not opened");    // не удалось открыть файл
            } else {
             // файл успешно открыт
                // создаем диалог с прогрессом и отменой
                ProgressAndSpeedDialog *pasd = new ProgressAndSpeedDialog(this);
                pasd->setWindowTitle( QString("Запись файла %1").
                                        arg(QDir::toNativeSeparators(fullName)) );

                // CОЕДИНЯЕМ:
                // сигнал о текущем прогрессе от генератора к визуализатору
                QObject::connect( generator, &FileGenerator::currentProgress,
                                  pasd,      &ProgressAndSpeedDialog::setProgress );
                // сигнал о завершении со слотом удаления диалога
                QObject::connect( generator, &FileGenerator::generationFinished,
                                  pasd,      &ProgressAndSpeedDialog::deleteLater );
                // сигнал от диалога о завершении с командой завершения генерации
                //     если объединить напрямую то ничего не произойдет -
                //     внутри одного потока слот не может прервать слот
                //     (т.к. не используем processEvents)
                QObject::connect( pasd, &ProgressAndSpeedDialog::abortProcess,
                                  [this]() {
                                        generator->abortGeneration();
                                  }
                                );

                // показываем диалог (он модальный, компоненты основной формы будут недоступны)
                pasd->show();

                // запускаем генерацию
                emit generateFile( file,                                            // файл
                                   fileSizeMb,                                      // размер
                                   ui->speedCBX->isChecked() ? 0 : writeSpeedMb_s );// скорость
            }
        }
        catch(...) {
            QMessageBox::critical( this,
                                   "Ошибка открытия файла!",
                                   QString( "Не удалось открыть файл "
                                            "\"%1\""
                                            " для записи.")
                                    .arg( QDir::toNativeSeparators(fullName) )
                                 );
        }
    }
}
//------------------------------------------------------------------------------
void Widget::generationFinished(FileGenerator::FinishReason r)
{
    if( r != FileGenerator::FileReady ) {
     // не получилось создать файл....
        file->close();  // закрываем файл
        file->remove(); // удаляем недоделанный файл
    } else {
     // файл был успешно создан
        if( ui->fileNameCBX->isChecked() ) {
         // формируем новое случайное имя файла
            fileName = getTmpName();
            ui->fileNameLED->setText( fileName );   // перенос в пользовательский интерфейс
        }
    }
    delete file; file = nullptr;
}
//------------------------------------------------------------------------------

