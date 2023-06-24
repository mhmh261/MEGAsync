#include "AddBackupDialog.h"
#include "ui_AddBackupDialog.h"
#include "UserAttributesRequests/DeviceName.h"
#include "UserAttributesRequests/MyBackupsHandle.h"
#include "Utilities.h"
#include "Platform.h"
#include "DialogOpener.h"

#include "QMegaMessageBox.h"

AddBackupDialog::AddBackupDialog(QWidget *parent) :
    QDialog(parent),
    mUi(new Ui::AddBackupDialog),
    mSelectedFolder(),
    mMyBackupsFolder(),
    mDeviceNameRequest (UserAttributes::DeviceName::requestDeviceName())
{
    mUi->setupUi(this);

    connect(mDeviceNameRequest.get(), &UserAttributes::DeviceName::attributeReady,
            this, &AddBackupDialog::onDeviceNameSet);
    onDeviceNameSet(mDeviceNameRequest->getDeviceName());    

#ifdef Q_OS_MACOS
    // Display our modal dialog embedded title label when parent is set
    mUi->embeddedTitleLabel->setVisible(this->parent() != nullptr);
#endif

    connect(mUi->addButton, &QPushButton::clicked, this, &AddBackupDialog::checkNameConflict);
    connect(mUi->cancelButton, &QPushButton::clicked, this, &QDialog::reject);
}

AddBackupDialog::~AddBackupDialog()
{
    delete mUi;
}

QString AddBackupDialog::getSelectedFolder()
{
    return mSelectedFolder;
}

QString AddBackupDialog::getBackupName()
{
    return mBackupName;
}

void AddBackupDialog::on_changeButton_clicked()
{
    auto processPath = [this](QStringList folderPaths)
    {
        if (!folderPaths.isEmpty())
        {
            auto folderPath = folderPaths.first();

            QString candidateDir (QDir::toNativeSeparators(QDir(folderPath).canonicalPath()));
            QString warningMessage;
            auto syncability (SyncController::isLocalFolderSyncable(candidateDir, mega::MegaSync::TYPE_BACKUP, warningMessage));

            if (syncability == SyncController::CANT_SYNC)
            {
                QMegaMessageBox::warning(nullptr, QString(), warningMessage, QMessageBox::Ok);
            }
            else if (syncability == SyncController::CAN_SYNC
                     || (syncability == SyncController::WARN_SYNC
                         && QMegaMessageBox::warning(nullptr, QString(), warningMessage
                                                     + QLatin1Char('/')
                                                     + tr("Do you want to continue?"),
                                                     QMessageBox::Yes | QMessageBox::No, QMessageBox::No)
                         == QMessageBox::Yes))
            {
                mSelectedFolder = candidateDir;
                mUi->folderLineEdit->setText(folderPath);
                mUi->addButton->setEnabled(true);
            }
        }
    };

    QString defaultPath = mUi->folderLineEdit->text().trimmed();
    if (!defaultPath.size())
    {
        defaultPath = Utilities::getDefaultBasePath();
    }

    defaultPath = QDir::toNativeSeparators(defaultPath);
    Platform::getInstance()->folderSelector(tr("Choose folder"), defaultPath,false, this, processPath);
}

void AddBackupDialog::onDeviceNameSet(const QString &devName)
{
    mUi->backupToLabel->setText(UserAttributes::MyBackupsHandle::getMyBackupsLocalizedPath()
                                + QLatin1Char('/')
                                + devName);
}

void AddBackupDialog::checkNameConflict()
{
    QStringList pathList;
    pathList.append(mSelectedFolder);

    if(!BackupNameConflictDialog::backupNamesValid(pathList))
    {
        BackupNameConflictDialog* conflictDialog = new BackupNameConflictDialog(pathList, this);
        DialogOpener::showDialog<BackupNameConflictDialog>(conflictDialog, this, &AddBackupDialog::onConflictSolved);
    }
    else
    {
        accept();
    }
}

void AddBackupDialog::onConflictSolved(QPointer<BackupNameConflictDialog> dialog)
{
    if(dialog->result() == QDialog::Accepted)
    {
        QMap<QString, QString> changes = dialog->getChanges();
        for(auto it = changes.cbegin(); it!=changes.cend(); ++it)
        {
            mBackupName = it.value();
        }
        accept();
    }
}
