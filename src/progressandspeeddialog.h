#ifndef PROGRESSANDSPEEDDIALOG_H
#define PROGRESSANDSPEEDDIALOG_H
//------------------------------------------------------------------------------
#include <QDialog>
//------------------------------------------------------------------------------
namespace Ui {
class ProgressAndSpeedDialog;
}
//------------------------------------------------------------------------------
class ProgressAndSpeedDialog : public QDialog
{
    Q_OBJECT

public:
    explicit                    ProgressAndSpeedDialog(QWidget *parent = nullptr);
                               ~ProgressAndSpeedDialog();

signals:
    void                        abortProcess();

public slots:
    void                        setProgress( int progress, double speedMb );

private slots:
    void                        on_resultBBX_rejected();

private:
    Ui::ProgressAndSpeedDialog  *ui;
    bool                         rejectFlag;

protected:
    void                        closeEvent(QCloseEvent *event);
    void                        hideEvent(QHideEvent *event);
};
//------------------------------------------------------------------------------
#endif // PROGRESSANDSPEEDDIALOG_H
