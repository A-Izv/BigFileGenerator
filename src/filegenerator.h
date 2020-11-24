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
    void                    currentProgress( int p, double curSpeed );

public slots:
    void                    generateFile(QFile *file, int sizeMb, int speedMb );

private:
    static const int        MAX_SIZE;
    static const int        MIN_SIZE;

private:
    Ipp32f                  *buffer;
    IppsRandGaussState_32f  *randGaussState;
    double                  cpuFreq;
    bool                    abortFlag;

};
//------------------------------------------------------------------------------
#endif // FILEGENERATOR_H
