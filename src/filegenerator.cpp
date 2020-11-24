#include "filegenerator.h"
//------------------------------------------------------------------------------
#include <QFile>
#include "ippCustom.h"
#include "appGlobal.h"
#include <cmath>
//------------------------------------------------------------------------------
const int FileGenerator::MAX_SIZE = 64*1024*1024/sizeof(float);
const int FileGenerator::MIN_SIZE = 1024/sizeof(float);
//------------------------------------------------------------------------------
FileGenerator::FileGenerator(QObject *parent) :
    QObject( parent ),
    buffer( nullptr ),
    randGaussState( nullptr ),
    abortFlag(false)
{
 // регистрация своего типа
    // чтобы использовать свой тип данныъ в сигнал/слотовых соединениях
    // объектов из разных потоков, тип надо зарегистрировать
    qRegisterMetaType<FinishReason>( "FinishReason" );

 // выделение памяти
    buffer = ippsMalloc_32f( MAX_SIZE );

 // подготовка генератора случайных чисел
    unsigned    seed = unsigned(   qrand() | (qrand() << 16)   );
    int         tmpI;

    CHK( ippsRandGaussGetSize_32f( &tmpI ) );                       // узнаем, сколько нужно памяти
    randGaussState = (IppsRandGaussState_32f*)ippsMalloc_8u( tmpI );// выделяем память
    CHK( ippsRandGaussInit_32f(                                     // инициализируем состояние генератора
              randGaussState,                                       // инициализируемая память
              0, float( 1/sqrt(2) ),                                    // параметры формируемой последовательности
              seed ) );                                                 // начальное состояние генератора
 // получаем частоту ЦПУ в тактах
    CHK( ippGetCpuFreqMhz(&tmpI) );
    cpuFreq = tmpI * 1000. * 1000.;
}
//------------------------------------------------------------------------------
FileGenerator::~FileGenerator()
{
    ippsFree( buffer );
    ippsFree( randGaussState );
}
//------------------------------------------------------------------------------
void FileGenerator::abortGeneration()
{
    abortFlag = true;
}
//------------------------------------------------------------------------------
void FileGenerator::generateFile(QFile *file, int sizeMb, int speedMb )
{
    FinishReason result = Error;

    abortFlag = false; // сбрасываем флаг прекращения
    if( file && file->isOpen() ) {
        try {
            qint64  bytesToWrite   = sizeMb*1024*1024ll; // если не будет константы LL то результат будет int
            qint64  samplesToWrite = bytesToWrite / sizeof(Ipp32f);
            qint64  tmp64;
            quint64 bgnTime;
            qint64  bytesWrited = 0;
            int     portionSizeSmpl;
            double  speed;

         // настраиваем файл
            file->resize( 0 );  // изменяем размер файла
            file->seek( 0 );    // позиционируемся на начале файла
         // запоминаем начальную метку времени
            bgnTime = ippGetCpuClocks();
         // цикл записи
            while( samplesToWrite &&    // пока не запишем все отсчеты
                  !abortFlag )          // или пока не будет выставлен флаг
            {
                // определяем размер порции в зависимости от заданной скорости
                portionSizeSmpl = qMin<qint64>( MAX_SIZE, samplesToWrite );
                // формируем ПСП
                CHK( ippsRandGauss_32f( buffer, portionSizeSmpl, randGaussState ) );
                // записываем ПСП в файл
                tmp64 = file->write( (char*)buffer, portionSizeSmpl*sizeof(Ipp32f) );
                bytesWrited    += tmp64;
                samplesToWrite -= tmp64 / sizeof(Ipp32f);
                // расчитываем текущую скорость
                tmp64 = ippGetCpuClocks() - bgnTime;    // число тактов, прошедшее с начала
                speed = bytesWrited/double(1024*1024);  // число записанных мегабайт
                speed = speed / (tmp64/cpuFreq);        // скорость, мегабайт в секунду
                // отправляем сигнал с информацией о прогрессе и скорости
                emit currentProgress( 100LL*bytesWrited/bytesToWrite, speed );
            }
            result = abortFlag ? HasBeenAborted : FileReady; // если записали все, что нужно - отправляем FileReady
        }
        catch(...) {
            result = Error;
        }
    }

    emit generationFinished( result );
}
//------------------------------------------------------------------------------


