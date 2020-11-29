#ifndef FILEGENERATOR_H
#define FILEGENERATOR_H
//------------------------------------------------------------------------------
#include <QObject>
//------------------------------------------------------------------------------
// включаем генерацию исключения как реакцию на ошибку IPP
#ifndef IPP_CHK_WITH_EXCEPTIONS
    #define IPP_CHK_WITH_EXCEPTIONS
#endif
#include "ippCustom.h"
//------------------------------------------------------------------------------
class QFile;
//------------------------------------------------------------------------------
class FileGenerator : public QObject
{
    Q_OBJECT
public:
    enum FinishReason{ Error, HasBeenAborted, FileReady };
public:
                            FileGenerator( QObject *parent = nullptr );
                           ~FileGenerator();

    void                    abortGeneration();

signals:
    void                    generationFinished( FinishReason r );
    void                    currentProgress( int p );

public slots:
    void                    generateFile(QFile *file,
                                         int sizeMb,
                                         short speedMb,
                                         int sinNum );

private:
    static const int        MAX_SIZE_SMPL = 64*1024*1024/sizeof(Ipp32fc);

private:
    Ipp32fc                 *sinBuf;        // буфер для формирования гармоник
    Ipp32fc                 *buffer;        // буфер для формирования ПСП
    IppsRandGaussState_32f  *randGaussState;// состояние гаусовского генератора случайных чисел
    qint64                  cpuFreq;        // частота процессора, МГц
    bool                    abortFlag;      // флаг (принудительного) завершения генерации

 };
//------------------------------------------------------------------------------
#endif // FILEGENERATOR_H
