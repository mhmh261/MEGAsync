#include "DownloadFromMegaDialog.h"
#include "ui_DownloadFromMegaDialog.h"
#include "control/Utilities.h"
#include "Platform.h"

#include <QDesktopServices>
#include <QTemporaryFile>
#include "QMegaMessageBox.h"
#include <QPointer>

DownloadFromMegaDialog::DownloadFromMegaDialog(QString defaultPath, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::DownloadFromMegaDialog)
{
    ui->setupUi(this);

    QString defaultDownloadPath;

    if (!defaultPath.size() || !QFile(defaultPath).exists())
    {
        defaultDownloadPath = Utilities::getDefaultBasePath() + QString::fromUtf8("/MEGAsync Downloads");
    }
    else
    {
        defaultDownloadPath = defaultPath;
    }

    ui->eFolderPath->setText(QDir::toNativeSeparators(defaultDownloadPath));
    ui->cDefaultPath->setChecked(false);
    ui->bChange->setEnabled(true);
    ui->bOK->setEnabled(true);
    ui->bOK->setDefault(true);
}

DownloadFromMegaDialog::~DownloadFromMegaDialog()
{
    delete ui;
}

bool DownloadFromMegaDialog::isDefaultDownloadOption()
{
    return ui->cDefaultPath->isChecked();
}

void DownloadFromMegaDialog::setDefaultDownloadOption(bool value)
{
    ui->cDefaultPath->setChecked(value);
}

QString DownloadFromMegaDialog::getPath()
{
    return ui->eFolderPath->text();
}

void DownloadFromMegaDialog::on_bChange_clicked()
{
    Platform::getInstance()->folderSelector(tr("Select local folder"),ui->eFolderPath->text(),false,this,[this](QStringList selection){
        if(!selection.isEmpty())
        {
            QString fPath = selection.first();
            onPathChanged(fPath);
        }
    });
}

void DownloadFromMegaDialog::onPathChanged(const QString& path)
{
    if (!path.size())
    {
        return;
    }

    QTemporaryFile test(path + QDir::separator());
    if (!test.open())
    {
        QMegaMessageBox::critical(nullptr, tr("Error"), tr("You don't have write permissions in this local folder."));
        return;
    }

    ui->eFolderPath->setText(path);
}

void DownloadFromMegaDialog::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange)
    {
        ui->retranslateUi(this);
    }
    QDialog::changeEvent(event);
}

void DownloadFromMegaDialog::on_bOK_clicked()
{
    if (ui->eFolderPath->text().size())
    {
        QDir dir(ui->eFolderPath->text());
        if (!dir.exists())
        {
            dir.mkpath(QString::fromUtf8("."));
        }

        QString qFilePath = QDir::fromNativeSeparators(ui->eFolderPath->text()); // QFile always wants `/` as separator
        QTemporaryFile test(qFilePath + QDir::separator());
        if (!test.open())
        {
            QMegaMessageBox::critical(nullptr, tr("Error"), tr("You don't have write permissions in this local folder."));
            return;
        }

        accept();
    }
}
