#include "filegenerator.h"
//------------------------------------------------------------------------------
#include <QFile>
#include "ippCustom.h"
#include "appGlobal.h"
#include <cmath>
#include <QThread>
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
 // получаем частоту ЦПУ в герцах
    CHK( ippGetCpuFreqMhz(&tmpI) );
    cpuFreq = tmpI;// * 1000. * 1000.; // сколько милионов тактов в секунду
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
            qint64  bytesWritten = 0;
            qint64  speedBytePerMTact = (1024.*1024.*speedMb)/cpuFreq;
            qint64  portionSizeSmpl;
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
                // сколько байт следовало бы записать на текущий момент,
                // чтобы достичь требуемой скорости записи
                tmp64  = ippGetCpuClocks() - bgnTime;       // число тактов, прошедшее с начал записи
                tmp64 /= 1000*1000;                         // сколько милионов тактов прошло
                portionSizeSmpl  = tmp64*speedBytePerMTact; // число байт, которое должно быть записано
                portionSizeSmpl -= bytesWritten;            // вычитаем записанные байты - получаем сколько нужно записать
                portionSizeSmpl /= sizeof(Ipp32f);          // переводим в число отсчетов
                // корректируем значение
                portionSizeSmpl = qMin<qint64>( portionSizeSmpl, MAX_SIZE ); // не больше максимального размера
                portionSizeSmpl = qMin(         portionSizeSmpl, samplesToWrite ); // не больше требуемого числа байт
                // совсем маленькие порции записывать не будем (если это не остаток)
                if( portionSizeSmpl < MIN_SIZE &&
                    portionSizeSmpl != samplesToWrite )
                {
                 // отдаем остатки кванта другим потокам...
                    QThread::yieldCurrentThread();
                } else {
                    // формируем ПСП
                    CHK( ippsRandGauss_32f( buffer, portionSizeSmpl, randGaussState ) );
                    // записываем ПСП в файл
                    tmp64 = file->write( (char*)buffer, portionSizeSmpl*sizeof(Ipp32f) );
                    bytesWritten   += tmp64;
                    samplesToWrite -= tmp64 / sizeof(Ipp32f); // !уменьшаем объем, который надо записать
                    // расчитываем среднюю скорость
                    tmp64 = ippGetCpuClocks() - bgnTime;            // число тактов, прошедшее с начала
                    speed = bytesWritten / (tmp64/(cpuFreq));       // скорость, мегабайт в секунду (т.к. cpuFreq в МГц)
                    // отправляем сигнал с информацией о прогрессе и скорости
                    emit currentProgress( 100LL*bytesWritten/bytesToWrite, speed );
                }
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


