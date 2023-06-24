#include "BugReportDialog.h"
#include "DialogOpener.h"

#include <TransfersModel.h>
#include <Preferences.h>

#include <QCloseEvent>
#include <QRegExp>

#include "ui_BugReportDialog.h"


using namespace mega;

BugReportDialog::BugReportDialog(QWidget *parent, MegaSyncLogger& logger) :
    QDialog(parent),
    logger(logger),
    ui(new Ui::BugReportDialog),
    mTransferFinished(false),
    mTransferError(MegaError::API_OK),
    mHadGlobalPause(false)
{
    ui->setupUi(this);

    ui->lDescribeBug->setText(ui->lDescribeBug->text() + QString::fromUtf8("<span style=\"color:red; text-decoration:none;\">*</span>"));
    ui->bSubmit->setDefault(true);
    ui->bSubmit->setEnabled(false);

    connect(ui->teDescribeBug, SIGNAL(textChanged()), this, SLOT(onDescriptionChanged()));
    connect(&logger, SIGNAL(logReadyForReporting()), this, SLOT(onReadyForReporting()));

    currentTransfer = 0;
    warningShown = false;
    errorShown = false;
    preparing = false;
    totalBytes = 0;
    transferredBytes = 0;
    lastpermil = -3;

    megaApi = ((MegaApplication *)qApp)->getMegaApi();
    delegateTransferListener = new QTMegaTransferListener(megaApi, this);
    delegateRequestListener = new QTMegaRequestListener(megaApi, this);
}

BugReportDialog::~BugReportDialog()
{
    //Just in case the dialog is closed from an exit action
    cancelCurrentReportUpload();

    delete ui;
    delete delegateTransferListener;
    delete delegateRequestListener;
}

void BugReportDialog::onTransferStart(MegaApi*, MegaTransfer* transfer)
{
    if (!currentTransfer)
    {
        currentTransfer = transfer->getTag();
    }

    totalBytes = transfer->getTotalBytes();
    mTransferError = MegaError::API_OK;
    mTransferFinished = false;

    mSendProgress = new QProgressDialog(this);

    connect(mSendProgress.data(), &QProgressDialog::canceled, this, &BugReportDialog::cancelSendReport);

    mSendProgress->setMinimumDuration(0);
    mSendProgress->setMinimum(0);
    mSendProgress->setMaximum(1010);
    mSendProgress->setValue(0);
    mSendProgress->setAutoClose(false);
    mSendProgress->setAutoReset(false);
    lastpermil = 0;

    auto labelWidget = new QLabel(tr("Bug report is uploading, it may take a few minutes"));
    labelWidget->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);
    labelWidget->setWordWrap(true);
    labelWidget->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
    mSendProgress->setLabel(labelWidget);

    DialogOpener::showDialog(mSendProgress);
}

void BugReportDialog::onTransferUpdate(MegaApi*, MegaTransfer* transfer)
{
    if (mSendProgress && transfer->getState() == MegaTransfer::STATE_ACTIVE)
    {
        transferredBytes = transfer->getTransferredBytes();
        int permil = (totalBytes > 0) ? static_cast<int>((1000 * transferredBytes) / totalBytes) : 0;
        if (permil > lastpermil)
        {
            mSendProgress->setValue(permil);
            lastpermil = permil;
        }
    }
}

void BugReportDialog::onTransferFinish(MegaApi*, MegaTransfer*, MegaError* error)
{
    if(mHadGlobalPause)
    {
        MegaSyncApp->getTransfersModel()->setGlobalPause(true);
    }

    disconnect(mSendProgress.data(), &QProgressDialog::canceled, this, &BugReportDialog::cancelSendReport);

    mSendProgress->reset();
    totalBytes = 0;
    transferredBytes = 0;
    currentTransfer = 0;
    lastpermil = -3;

    mTransferError = error->getErrorCode();
    mTransferFinished = true;

    if (!warningShown)
    {
        postUpload();
    }

    logger.resumeAfterReporting();
}

void BugReportDialog::onTransferTemporaryError(MegaApi*, MegaTransfer*, MegaError *e)
{
    MegaApi::log(MegaApi::LOG_LEVEL_ERROR, QString::fromUtf8("Temporary error at report dialog: %1")
                 .arg(QString::fromUtf8(e->getErrorString())).toUtf8().constData());
}

void BugReportDialog::onRequestFinish(MegaApi*, MegaRequest* request, MegaError* e)
{
    switch(request->getType())
    {
        case MegaRequest::TYPE_SUPPORT_TICKET:
        {
            if (mSendProgress)
            {
                mSendProgress->close();
            }

            if (e->getErrorCode() == MegaError::API_OK)
            {
                QMessageBox* msgBox = new QMessageBox();
                msgBox->setWindowTitle(tr("Bug report"));
                msgBox->setIcon(QMessageBox::Information);
                msgBox->setText(tr("Bug report success!"));
                msgBox->setTextFormat(Qt::RichText);
                msgBox->setIconPixmap(QPixmap(Utilities::getDevicePixelRatio() < 2 ? QString::fromUtf8(":/images/bug_report_success.png")
                                                                                  : QString::fromUtf8(":/images/bug_report_success@2x.png")));
                msgBox->setInformativeText(tr("Your bug report has been submitted, a confirmation email will sent to you accordingly."));
                msgBox->addButton(tr("Ok"), QMessageBox::AcceptRole);

                DialogOpener::showDialog<QMessageBox>(msgBox, [this](){
                    close();
                });
            }
            else
            {
                showErrorMessage();
            }

            break;
        }
        default:
            break;
    }
}

void BugReportDialog::showErrorMessage()
{
    if (errorShown)
    {
        return;
    }

    QMessageBox* msgBox = new QMessageBox();
    msgBox->setWindowTitle(tr("Bug report"));
    msgBox->setText(tr("Error on submitting bug report"));
    msgBox->setTextFormat(Qt::RichText);
    msgBox->addButton(tr("Ok"), QMessageBox::AcceptRole);

    if (mTransferFinished && mTransferError == MegaError::API_EEXIST)
    {
        msgBox->setIcon(QMessageBox::Information);
        msgBox->setInformativeText(tr("There is an ongoing report being uploaded.")
                                  + QString::fromUtf8("<br>") +
                                  tr("Please wait until the current upload is completed."));
    }
    else
    {
        msgBox->setIcon(QMessageBox::Warning);
        msgBox->setInformativeText(
                    tr("Bug report can't be submitted due to some error. Please try again or contact our support team via [A]support@mega.co.nz[/A]")
                        .replace(QString::fromUtf8("[A]"), QString::fromUtf8("<span style=\"font-weight: bold; text-decoration:none;\">"))
                        .replace(QString::fromUtf8("[/A]"), QString::fromUtf8("</span>"))
                         + QString::fromAscii("\n"));
    }

    errorShown = true;
    DialogOpener::showDialog<QMessageBox>(msgBox, [this](){
        errorShown = false;
        close();
    });
}

void BugReportDialog::postUpload()
{
    if (mTransferError == MegaError::API_OK)
    {
        if(mSendProgress)
        {
            mSendProgress->setValue(mSendProgress->maximum());
        }
        createSupportTicket();
    }
    else
    {
        if(mSendProgress)
        {
            mSendProgress->close();
        }
        showErrorMessage();
    }
}

void BugReportDialog::createSupportTicket()
{
    QString report = QString::fromUtf8("Version string: %1   Version code: %2.%3   User-Agent: %4\n").arg(Preferences::VERSION_STRING)
                                    .arg(Preferences::VERSION_CODE)
                                    .arg(Preferences::BUILD_ID)
                                    .arg(QString::fromUtf8(megaApi->getUserAgent()));

    report.append(QString::fromUtf8("Report filename: %1").arg(reportFileName.isEmpty() ? QString::fromUtf8("Not sent") : reportFileName).append(QString::fromUtf8("\n")));
    report.append(QString::fromUtf8("Title: %1").arg(ui->leTitleBug->text().append(QString::fromUtf8("\n"))));
    report.append(QString::fromUtf8("Description: %1").arg(ui->teDescribeBug->toPlainText().append(QString::fromUtf8("\n"))));

    megaApi->createSupportTicket(report.toUtf8().constData(), 6, delegateRequestListener);
}

void BugReportDialog::cancelCurrentReportUpload()
{
    if (!mTransferFinished && currentTransfer)
    {
        megaApi->cancelTransferByTag(currentTransfer);

        if(mHadGlobalPause)
        {
            MegaSyncApp->getTransfersModel()->setGlobalPause(true);
        }
    }
}

void BugReportDialog::on_bSubmit_clicked()
{
    if (preparing)
    {
        return;
    }

    if (logger.prepareForReporting())
    {
        preparing = true;
    }
}

void BugReportDialog::onReadyForReporting()
{
    reportFileName.clear();

    //If send log file is enabled
    if (ui->cbAttachLogs->isChecked())
    {
        QString pathToLogFile = Utilities::joinLogZipFiles(megaApi);
        if (pathToLogFile.isNull())
        {
            showErrorMessage();
            logger.resumeAfterReporting();
            preparing = false;
        }
        else
        {
            QFileInfo joinLogsFile{pathToLogFile};
            reportFileName = joinLogsFile.fileName();
            if(Preferences::instance()->getGlobalPaused())
            {
                mHadGlobalPause = true;
                MegaSyncApp->getTransfersModel()->setGlobalPause(false);
            }
            megaApi->startUploadForSupport(QDir::toNativeSeparators(pathToLogFile).toUtf8().constData(), true, delegateTransferListener);
        }
    }
    else
    {
        //Create support ticket
        createSupportTicket();
    }
}

void BugReportDialog::on_bCancel_clicked()
{
    reject();
}

void BugReportDialog::cancelSendReport()
{
    if (warningShown)
    {
        return;
    }

    MegaSyncApp->getTransfersModel()->pauseResumeTransferByTag(currentTransfer, true);

    QMessageBox* msgBox = new QMessageBox();
    msgBox->setIcon(QMessageBox::Warning);
    msgBox->setWindowTitle(tr("Bug report"));
    msgBox->setText(tr("Are you sure you want to exit uploading?"));
    msgBox->setTextFormat(Qt::RichText);
    msgBox->setInformativeText(tr("The bug report will not be submitted if you exit uploading."));

    msgBox->addButton(tr("Continue"), QMessageBox::AcceptRole);
    msgBox->addButton(tr("Yes"), QMessageBox::RejectRole);

    warningShown = true;
    DialogOpener::showDialog<QMessageBox>(msgBox, [this, msgBox]{
        int ret = msgBox->result();

        if (ret == QMessageBox::RejectRole)
        {
            cancelCurrentReportUpload();
            preparing = false;
            warningShown = false;
            reject();
        }
        else if (ret == QMessageBox::AcceptRole)
        {
            warningShown = false;
            if (mTransferFinished)
            {
                postUpload();
            }
            else if (currentTransfer && mSendProgress)
            {
                DialogOpener::showDialog(mSendProgress);
                mSendProgress->setValue(lastpermil);
                MegaSyncApp->getTransfersModel()->pauseResumeTransferByTag(currentTransfer,false);
            }
        }});
}

void BugReportDialog::onDescriptionChanged()
{
    ui->bSubmit->setEnabled(!ui->teDescribeBug->toPlainText().isEmpty());
}

void BugReportDialog::on_teDescribeBug_textChanged()
{
    if(ui->teDescribeBug->toPlainText().length() > mMaxDescriptionLength)
    {
        int diff = ui->teDescribeBug->toPlainText().length() - mMaxDescriptionLength; //m_maxTextEditLength - just an integer
        QString newStr = ui->teDescribeBug->toPlainText();
        newStr.chop(diff);
        ui->teDescribeBug->setText(newStr);
        QTextCursor cursor(ui->teDescribeBug->textCursor());
        cursor.movePosition(QTextCursor::End, QTextCursor::MoveAnchor);
        ui->teDescribeBug->setTextCursor(cursor);
    }
}
