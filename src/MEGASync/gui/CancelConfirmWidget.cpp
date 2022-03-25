#include "CancelConfirmWidget.h"
#include "ui_CancelConfirmWidget.h"

CancelConfirmWidget::CancelConfirmWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CancelConfirmWidget)
{
    ui->setupUi(this);
}

CancelConfirmWidget::~CancelConfirmWidget()
{
    delete ui;
}

void CancelConfirmWidget::show()
{
    enableButtons(true);
}

void CancelConfirmWidget::on_pDismiss_clicked()
{
    emit dismiss();
}

void CancelConfirmWidget::on_pProceed_clicked()
{
    enableButtons(false);
    emit proceed();
}

void CancelConfirmWidget::enableButtons(bool value)
{
    ui->pDismiss->setEnabled(value);
    ui->pProceed->setEnabled(value);
}
