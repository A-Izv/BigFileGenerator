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
    ui->resultBBX->button( QDialogButtonBox::Cancel )->setText("Отмена");
    setWindowFlag( Qt::WindowContextHelpButtonHint, false );


    rejectFlag = false;
}
//------------------------------------------------------------------------------
ProgressAndSpeedDialog::~ProgressAndSpeedDialog()
{
    qDebug() << "ProgressAndSpeedDialog:" << "destructor";
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
    if( !rejectFlag ) {
        rejectFlag = true;
        qDebug() << "ProgressAndSpeedDialog:" << "reject";

        emit abortProcess();
        deleteLater();
    }
}
//------------------------------------------------------------------------------
void ProgressAndSpeedDialog::closeEvent(QCloseEvent *event)
{
    qDebug() << "ProgressAndSpeedDialog:" << "closeEvent";
    QDialog::closeEvent(event);
}
//------------------------------------------------------------------------------
void ProgressAndSpeedDialog::hideEvent(QHideEvent *event)
{
    qDebug() << "ProgressAndSpeedDialog:" << "hideEvent";
    on_resultBBX_rejected();
    QDialog::hideEvent(event);
}
//------------------------------------------------------------------------------
