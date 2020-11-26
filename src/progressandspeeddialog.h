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
    void                        abortProcess(); // диалог может послать сигнал о досрочном завершении

public slots:
    void                        setProgress( int progress ); // отображаемое значение - снаружи

private slots:
    void                        on_resultBBX_rejected();

protected:
    Ui::ProgressAndSpeedDialog  *ui;
    bool                        rejectFlag; // флаг для того, чтобы сигнал отправлялся лишь раз

    void                        rejection();

    void                        hideEvent(QHideEvent *event);
};
//------------------------------------------------------------------------------
#endif // PROGRESSANDSPEEDDIALOG_H
