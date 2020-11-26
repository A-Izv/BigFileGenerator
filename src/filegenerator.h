#ifndef FILEGENERATOR_H
#define FILEGENERATOR_H
//------------------------------------------------------------------------------
#include <QObject>
#include "ipps.h"
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
    void                    generateFile(QFile *file, int sizeMb, short speedMb );

private:
    static const int        MAX_SIZE_SMPL;

private:
    Ipp32f                  *buffer;        // буфер для формирования ПСП
    IppsRandGaussState_32f  *randGaussState;// состояние гаусовского генератора случайных чисел
    qint64                  cpuFreq;        // частота процессора, МГц
    bool                    abortFlag;      // флаг (принудительного) завершения генерации

};
//------------------------------------------------------------------------------
#endif // FILEGENERATOR_H
