#include "progressandspeeddialog.h"
#include "ui_progressandspeeddialog.h"
#include <QDebug>
#include <QPushButton>
//------------------------------------------------------------------------------
ProgressAndSpeedDialog::ProgressAndSpeedDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProgressAndSpeedDialog)
{
    ui->setupUi(this);

    // фиксируем размер окна по вертикали
    this->setMaximumHeight( this->layout()->minimumSize().height() );

    // изменяем надпись на кнопке
    ui->resultBBX->button( QDialogButtonBox::Cancel )->setText("Отмена");

    // убираем кнопку "help"
    setWindowFlag( Qt::WindowContextHelpButtonHint, false );

    rejectFlag = false; // сброс флага, предотвращающий многокртаную отправку сигнала
}
//------------------------------------------------------------------------------
ProgressAndSpeedDialog::~ProgressAndSpeedDialog()
{
    delete ui;
}
//------------------------------------------------------------------------------
void ProgressAndSpeedDialog::setProgress(int progress, double speedMb)
{
    ui->PBR->setValue( progress );
    ui->speedLBL->setText( QString::number(speedMb,'f',3) );
}
//------------------------------------------------------------------------------
void ProgressAndSpeedDialog::on_resultBBX_rejected()
{
    rejection(); // при нажатии на кнопку - выполняем отмену
}
//------------------------------------------------------------------------------
void ProgressAndSpeedDialog::rejection()
{
    if( !rejectFlag ) {
        rejectFlag = true;      // теперь сигнал будет отправлен лишь раз

        emit abortProcess();    // отправляем сигнал о досрочном завершении
    }
}
//------------------------------------------------------------------------------
void ProgressAndSpeedDialog::hideEvent(QHideEvent *event)
{
    // hide произойдет при любом способе закрытия диалога:
    //  - нажатии на кнопку Esc
    //  - нажатии на "крестик"
    //  - удалении объекта класса (снаружи)
    rejection();                // выполняем отмену
    QDialog::hideEvent(event);  // выполняем действия по умолчанию для события hide
}
//------------------------------------------------------------------------------
