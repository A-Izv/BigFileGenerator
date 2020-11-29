#include "filegenerator.h"
//------------------------------------------------------------------------------
#include <QFile>
#include <cmath>
#include <QThread>
//------------------------------------------------------------------------------
FileGenerator::FileGenerator(QObject *parent) :
    QObject( parent ),
    sinBuf( nullptr ),
    buffer( nullptr ),
    randGaussState( nullptr ),
    abortFlag(false)
{
 // регистрация своего типа
    // чтобы использовать свой тип данныъ в сигнал/слотовых соединениях
    // объектов из разных потоков, тип надо зарегистрировать
    qRegisterMetaType<FinishReason>( "FinishReason" );

 // выделение памяти
    buffer = ippsMalloc_32fc( MAX_SIZE_SMPL );
    sinBuf = ippsMalloc_32fc( MAX_SIZE_SMPL );

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
    cpuFreq = tmpI; // сколько милионов тактов в секунду
}
//------------------------------------------------------------------------------
FileGenerator::~FileGenerator()
{
    ippsFree( buffer );
    ippsFree( sinBuf );
    ippsFree( randGaussState );
}
//------------------------------------------------------------------------------
void FileGenerator::abortGeneration()
{
    abortFlag = true;
}
//------------------------------------------------------------------------------
void FileGenerator::generateFile(QFile *file, int sizeMb, short speedMb, int sinNum )
{
    FinishReason result = Error;

    abortFlag = false; // сбрасываем флаг прекращения
    if( file && file->isOpen() ) {
        try {
            qint64  bytesToWrite   = 1024LL*1024*sizeMb; // если не будет константы LL то результат будет int
            qint64  samplesToWrite = bytesToWrite / sizeof(Ipp32fc);
            qint64  tmp64;
            qint64  curTime, bgnTime, oldTime;
            qint64  bytesWritten = 0;
            qint64  speedBytePerMTact = (1024LL*1024*speedMb)/cpuFreq; // LL нужна!
            qint64  portionSizeSmpl;
            float   phase = 0;

         // настраиваем файл
            file->resize( 0 );  // изменяем размер файла
            file->seek( 0 );    // позиционируемся на начале файла
         // запоминаем начальную метку времени
            bgnTime = oldTime = ippGetCpuClocks();
         // цикл записи
            while( samplesToWrite &&    // пока не запишем все отсчеты
                  !abortFlag )          // или пока не будет выставлен флаг
            {
                // сколько байт следовало бы записать на текущий момент,
                // чтобы достичь требуемой скорости записи
                curTime = ippGetCpuClocks();
                tmp64   = curTime - bgnTime;                // число тактов, прошедшее с начал записи
                tmp64  /= 1000*1000;                        // сколько милионов тактов прошло
                portionSizeSmpl  = tmp64*speedBytePerMTact; // число байт, которое должно быть записано
                portionSizeSmpl -= bytesWritten;            // вычитаем записанные байты - получаем сколько еще нужно записать
                portionSizeSmpl /= sizeof(Ipp32fc);         // переводим в число отсчетов
                // корректируем значение
                portionSizeSmpl = qMin<qint64>( portionSizeSmpl, MAX_SIZE_SMPL  ); // не больше максимального размера
                portionSizeSmpl = qMin(         portionSizeSmpl, samplesToWrite ); // не больше требуемого числа байт

                // вместо записи данныъ будем спать если:
                if( portionSizeSmpl < MAX_SIZE_SMPL     &&  // порция меньше максимальной
                    portionSizeSmpl != samplesToWrite   &&  // это не остаток (последняя порция данных)
                   (curTime-oldTime)/1000000 < cpuFreq )    // последняя запись была меньше чем секунду назад
                {
                 // спим 10 милисекунд
                    QThread::msleep(10);
                } else {
                    oldTime = curTime; // запоминаем время последней записи

                    // формируем ПСП
                    CHK( ippsRandGauss_32f( (Ipp32f*)buffer, 2*portionSizeSmpl, randGaussState ) );
                    // добавляем гармоник
                    for( int i = 0 ; i < sinNum ; ++i ) {
                        CHK( ippsTone_32fc( sinBuf, portionSizeSmpl,                // формируем гармонику
                                            100, i*(1./sinNum), &phase,
                                            ippAlgHintFast ) );
                        CHK( ippsAdd_32fc_I( sinBuf, buffer, portionSizeSmpl ) );   // добавляем в реализацию
                    };
                    // записываем реализацию в файл
                    tmp64 = file->write( (char*)buffer, portionSizeSmpl*sizeof(Ipp32fc) );
                    bytesWritten   += tmp64;
                    samplesToWrite -= tmp64 / sizeof(Ipp32fc); // !уменьшаем объем, который надо записать

                    // отправляем сигнал с информацией о прогрессе
                    emit currentProgress( 100*bytesWritten/bytesToWrite );
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


