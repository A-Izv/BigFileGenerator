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
#include <windows.h>
#include <fcntl.h>
//------------------------------------------------------------------------------
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

    ui->realizeBGR->setId( ui->noiseRBN,       Noise );
    ui->realizeBGR->setId( ui->noiseAndSinRBN, NoiseAndSin );

    QObject::connect( ui->realizeBGR,
                      QOverload<int>::of(&QButtonGroup::buttonPressed),
                      [this](int id) { realizeType = (RealizeType)id; } );

    this->setWindowTitle( QString("Файлоформирователь (%1)").arg(VERSION) );
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
    // тип реализации
    realizeType = (RealizeType)settings.value( "realizeType", Noise ).toInt();
    ui->realizeBGR->button( realizeType )->setChecked(true);
    // число гармоник в реализации
    sinNumber = settings.value( "sinNumber", 1 ).toInt();
    ui->sinHSL->setValue( sinNumber );
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
    settings.setValue( "realizeType",       realizeType );
    settings.setValue( "sinNumber",         sinNumber );

 // удаляем объекты
    thread->quit(); // просим поток завершиться
    thread->wait(); // ожидаем завершения потока
    delete  thread;     thread      = nullptr;  // корректно удаляется лишь завершенный поток

    delete generator;   generator   = nullptr;  // теперь можно удалить и генератор

    delete file;        file        = nullptr;  // и генерируемый файл

    delete ui;
}
//------------------------------------------------------------------------------
void Widget::closeEvent(QCloseEvent *event)
{// главное окно приложения закрывают
    generator->abortGeneration();
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

    QFileInfo fi(result);

 // проверяем, существует ли файл
    while( flag && fi.exists(result) ) {
        if( fi.isFile() ) {
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
                        fi.setFile( result );
                        // флаг не сбрасываем, чтобы еще раз проверить наличие файла
                        break;
             // любой другой исход - отмена
                default :
                        result.clear();
                        flag = false;
            }
            delete box;
        } else {
            QMessageBox::warning( this,
                                  "Невозможно создать файл",
                                  QString( "Невозможно создать файл \"%1\", "
                                           "т.к. такой объект уже существует." ).
                                            arg(QDir::toNativeSeparators(result)) );
            result.clear();
            flag = false;
        }
    }
    return result;
}
//------------------------------------------------------------------------------
void Widget::on_startPBN_clicked()
{
    QString fullName = getFullPathName();

 // создаем объект для доступа к файлу
    if( !fullName.isEmpty() ) {
        file = new QFile; // создаем объект для доступа к файлу

     // Qt открывает файлы с доступом на чтение/запись для других процессов.
     // В Windows (если этого не нужно) чаще всего запрещают такой доступ.
     // Для имитации такого подхода открывать файл будем с помощью WinAPI,
     // а потом полученный дескриптор использовать в QFile.
        wchar_t wName[1000] = {0};  // строка многобайтных символов для WinAPI
        HANDLE  fileH = 0;          // дескриптор файла WinAPI

        fullName.toWCharArray( wName ); // формируем имя файла в буфере

        // открываем файл
        fileH = CreateFile( wName,                   // pointer to name of the file
                            GENERIC_WRITE,           // access (read-write) mode
                            0,                       // share mode ( 0 - no share )
                            NULL,                    // pointer to security attributes
                            CREATE_ALWAYS,           // how to create
                            FILE_ATTRIBUTE_NORMAL,   // file attributes
                            NULL                     // handle to file with attributes to copy
                            );
        // преобразуем дескриптор из нативного (WinAPI) в дескриптор Си
        // (он нужен для стандартных, а не API-шные, функций работы с файлами)
        auto fd = _open_osfhandle((intptr_t)fileH, 0 );

        try {
         // открываем файл на основе созданного дескриптора (со всеми его разрешениями)
            if( fd == -1 || !file->open( fd, QIODevice::WriteOnly, QFileDevice::AutoCloseHandle ) ) {
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
                emit generateFile( file,                                                 // файл
                                   fileSizeMb,                                           // размер
                                   ui->speedCBX->isChecked() ? 0x7FFF : writeSpeedMb_s,  // скорость, Мб/с
                                   realizeType == NoiseAndSin ? sinNumber : 0 );         // число гармоник
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
    QString fullName = pathName + QDir::separator() + fileName;

    if( r == FileGenerator::FileReady ) {
     // файл был успешно создан
        if( ui->fileNameCBX->isChecked() ) {
         // формируем новое случайное имя файла
            fileName = getTmpName();
            ui->fileNameLED->setText( fileName );   // перенос в пользовательский интерфейс
        }
    }

    delete file; file = nullptr;

    if( r != FileGenerator::FileReady ) {
     // так как целиком создать файл не получилось - удаляем
        QFile::remove( fullName );
    }
    if( r == FileGenerator::Error ) {
        QMessageBox::critical( this,
                               "Ошибка генерации",
                               QString( "При формировании файла \"%1\""
                                        "возникла ошибка." )
                                .arg( QDir::toNativeSeparators(fullName) )
                             );
    }
}
//------------------------------------------------------------------------------
void Widget::on_sinHSL_valueChanged(int value)
{
    sinNumber = value;
}
//------------------------------------------------------------------------------
