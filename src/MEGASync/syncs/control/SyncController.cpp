#include "SyncController.h"
#include "MegaApplication.h"
#include "Platform.h"
#include "UserAttributesRequests/MyBackupsHandle.h"

#include "mega/types.h"

#include <QStorageInfo>
#include <QTemporaryFile>

using namespace mega;

SyncController::SyncController(QObject* parent)
    : QObject(parent),
      mPendingBackups(QMap<QString, QString>()),
      mApi(MegaSyncApp->getMegaApi()),
      mDelegateListener (new QTMegaRequestListener(mApi, this)),
      mSyncInfo(SyncInfo::instance())
{
    // The controller shouldn't ever be instantiated before we have an API and a SyncInfo available
    assert(mApi);
    assert(mSyncInfo);
}

SyncController::~SyncController()
{
    delete mDelegateListener;
}

void SyncController::createPendingBackups()
{
    for(auto it = mPendingBackups.cbegin(); it != mPendingBackups.cend(); it++)
    {
        addSync(QDir::toNativeSeparators(it.key()), mega::INVALID_HANDLE,
                it.value().isEmpty() ? getSyncNameFromPath(it.key()) : it.value(),
                mega::MegaSync::TYPE_BACKUP);
    }
    mPendingBackups.clear();
}

void SyncController::addBackup(const QString& localFolder, const QString& syncName)
{
    mPendingBackups.insert(localFolder, syncName);

    auto request = UserAttributes::MyBackupsHandle::requestMyBackupsHandle();
    connect(request.get(), &UserAttributes::MyBackupsHandle::attributeReady, this, [this](){
      createPendingBackups();
    }, Qt::UniqueConnection);
    if(request->isAttributeReady())
    {
        createPendingBackups();
    }
    else
    {
        request->createMyBackupsFolderIfNeeded();
    }
}

void SyncController::addSync(const QString& localFolder, const MegaHandle& remoteHandle,
                             const QString& syncName, MegaSync::SyncType type)
{
    MegaApi::log(MegaApi::LOG_LEVEL_INFO, QString::fromUtf8("Adding sync (%1) \"%2\" for path \"%3\"")
                 .arg(getSyncTypeString(type), syncName, localFolder).toUtf8().constData());
    QString syncCleanName = syncName;
    syncCleanName.remove(Utilities::FORBIDDEN_CHARS_RX);
    mApi->syncFolder(type, localFolder.toUtf8().constData(),
                     syncCleanName.isEmpty() ? nullptr : syncCleanName.toUtf8().constData(),
                     remoteHandle, nullptr, mDelegateListener);
}

void SyncController::removeSync(std::shared_ptr<SyncSettings> syncSetting, const MegaHandle& remoteHandle)
{
    if (!syncSetting)
    {
        MegaApi::log(MegaApi::LOG_LEVEL_ERROR, QString::fromUtf8("Removing invalid sync").toUtf8().constData());
        return;
    }

    MegaApi::log(MegaApi::LOG_LEVEL_INFO, QString::fromUtf8("Removing sync (%1) \"%2\"")
                 .arg(getSyncTypeString(syncSetting->getType()), syncSetting->name()).toUtf8().constData());

    bool isBackup = syncSetting->getType() == MegaSync::TYPE_BACKUP;
    MegaHandle backupRoot = syncSetting->getMegaHandle();
    MegaHandle backupId = syncSetting->backupId();

    mApi->removeSync(backupId, new OnFinishOneShot(mApi, [=](const MegaError& e){
        if (e.getErrorCode() != MegaError::API_OK)
        {
            QString errorMsg = QString::fromUtf8(e.getErrorString());
            std::shared_ptr<SyncSettings> sync = mSyncInfo->getSyncSettingByTag(backupId);
            QString logMsg = QString::fromUtf8("Error removing sync (%1) (request error): %2 (sync id): %3").arg(
                                 getSyncTypeString(sync ? sync->getType() : MegaSync::SyncType::TYPE_UNKNOWN),
                                 errorMsg, QString::number(backupId));
            MegaApi::log(MegaApi::LOG_LEVEL_ERROR, logMsg.toUtf8().constData());
            emit syncRemoveError(std::shared_ptr<MegaError>(e.copy()));
        }
        else if (isBackup)
        {
            // We now have to delete or remove the remote folder
            mApi->moveOrRemoveDeconfiguredBackupNodes(backupRoot, remoteHandle,
                                                      new OnFinishOneShot(mApi, [=](const MegaError& e){
                if (e.getErrorCode() != MegaError::API_OK)
                {
                    QString errorMsg = QString::fromUtf8(e.getErrorString());
                    QString logMsg = QString::fromUtf8("Error moving or deleting remote backup folder (request error): %1 (sync id): %2 (Folder handle):%3").arg(
                                         errorMsg, QString::number(backupId), QString::number(backupRoot));
                    MegaApi::log(MegaApi::LOG_LEVEL_ERROR, logMsg.toUtf8().constData());
                    emit backupMoveOrRemoveRemoteFolderError(std::shared_ptr<MegaError>(e.copy()));
                }
                else
                {
                    QString logMsg = QString::fromUtf8("Remote backup folder correctly moved or removed. (Backup id): %1 (Folder handle): %2").arg(
                                        QString::number(backupId), QString::number(backupRoot));
                    MegaApi::log(MegaApi::LOG_LEVEL_INFO, logMsg.toUtf8().constData());
                }
            }));
        }
    }));
}

void SyncController::enableSync(std::shared_ptr<SyncSettings> syncSetting)
{
    if (!syncSetting)
    {
        MegaApi::log(MegaApi::LOG_LEVEL_ERROR, QString::fromUtf8("Trying to enable null sync").toUtf8().constData());
        return;
    }

    MegaApi::log(MegaApi::LOG_LEVEL_INFO, QString::fromUtf8("Enabling sync (%1) \"%2\" to \"%3\"")
                 .arg(getSyncTypeString(syncSetting->getType()), syncSetting->getLocalFolder(), syncSetting->getMegaFolder())
                 .toUtf8().constData());

    mApi->setSyncRunState(syncSetting->backupId(), MegaSync::RUNSTATE_RUNNING,
                          new OnFinishOneShot(mApi, [=](const MegaError& e){
        int errorCode (e.getErrorCode());
        auto syncErrorCode (static_cast<MegaSync::Error>(e.getSyncError()));

        if (syncSetting && (syncErrorCode != MegaSync::NO_SYNC_ERROR))
        {
            QString errorMsg = QString::fromUtf8("Error enabling sync (%1) \"%2\" for \"%3\" to \"%4\": %5").arg(
                        getSyncTypeString(syncSetting->getType()),
                        syncSetting->name(),
                        syncSetting->getLocalFolder(),
                        syncSetting->getMegaFolder(),
                        QString::fromUtf8(MegaSync::getMegaSyncErrorCode(syncErrorCode)));
            MegaApi::log(MegaApi::LOG_LEVEL_ERROR, errorMsg.toUtf8().constData());

            emit syncEnableError(syncSetting, syncErrorCode);
        }
        else
        {
            QString errorMsg = getSyncAPIErrorMsg(errorCode);
            if (errorMsg.isEmpty())
            {
                errorMsg = QString::fromUtf8(e.getErrorString());
            }

            QString logMsg = QString::fromUtf8("Error enabling sync (%1) (request reason): %2").arg(
                        getSyncTypeString(syncSetting ? syncSetting->getType() : MegaSync::SyncType::TYPE_UNKNOWN),
                        errorMsg);
            MegaApi::log(MegaApi::LOG_LEVEL_ERROR, logMsg.toUtf8().constData());
        }

        if (syncErrorCode == MegaSync::NO_SYNC_ERROR && syncSetting)
        {
            mSyncInfo->removeUnattendedDisabledSync(syncSetting->backupId(), syncSetting->getType());
        }
    }));
}

void SyncController::disableSync(std::shared_ptr<SyncSettings> syncSetting)
{
    if (!syncSetting)
    {
        MegaApi::log(MegaApi::LOG_LEVEL_ERROR, QString::fromUtf8("Trying to disable invalid sync").toUtf8().constData());
        return;
    }

    MegaApi::log(MegaApi::LOG_LEVEL_INFO, QString::fromUtf8("Disabling sync (%1) \"%2\" to \"%3\"")
                 .arg(getSyncTypeString(syncSetting->getType()), syncSetting->getLocalFolder(), syncSetting->getMegaFolder())
                 .toUtf8().constData());

    mApi->setSyncRunState(syncSetting->backupId(), MegaSync::RUNSTATE_DISABLED,
                          new OnFinishOneShot(mApi, [=](const MegaError& e){
        // NOTE: As of sdk commit 94e2b9dd1db6a886e21cc1ee826bda58c8c33f99, this never fails
        // and errorCode is always MegaError::API_OK.
        auto errorCode (e.getErrorCode());
        if (errorCode != MegaError::API_OK)
        {
            auto syncErrorCode (static_cast<MegaSync::Error>(e.getSyncError()));

            if (syncSetting && (syncErrorCode != MegaSync::NO_SYNC_ERROR))
            {
                QString logMsg = QString::fromUtf8("Error disabling sync (%1) \"%2\" for \"%3\" to \"%4\": %5").arg(
                            getSyncTypeString(syncSetting->getType()),
                            syncSetting->name(),
                            syncSetting->getLocalFolder(),
                            syncSetting->getMegaFolder(),
                            QString::fromUtf8(MegaSync::getMegaSyncErrorCode(syncErrorCode)));
                MegaApi::log(MegaApi::LOG_LEVEL_ERROR, logMsg.toUtf8().constData());

                emit syncDisableError(syncSetting, syncErrorCode);
            }
            else
            {
                QString errorMsg = getSyncAPIErrorMsg(errorCode);
                if (errorMsg.isEmpty())
                {
                    errorMsg = QString::fromUtf8(e.getErrorString());
                }

                QString logMsg = QString::fromUtf8("Error disabling sync (%1) (request error): %2").arg(
                            getSyncTypeString(syncSetting ? syncSetting->getType() : MegaSync::SyncType::TYPE_UNKNOWN),
                            errorMsg);
                MegaApi::log(MegaApi::LOG_LEVEL_ERROR, logMsg.toUtf8().constData());
            }
        }
    }));
}

// Checks if a path belongs is in an existing sync or backup tree; and if the selected
// folder has a sync or backup in its tree.
QString SyncController::getIsLocalFolderAlreadySyncedMsg(const QString& path, const MegaSync::SyncType& syncType)
{
    QString inputPath (QDir::toNativeSeparators(QDir(path).absolutePath()));
    QString message;

    // Gather all synced or backed-up dirs
    QMap<QString, MegaSync::SyncType> localFolders = SyncInfo::instance()->getLocalFoldersAndTypeMap();

    // Check if the path is already synced or part of a sync
    foreach (auto& existingPath, localFolders.keys())
    {
        if (inputPath == existingPath)
        {
            if (syncType == MegaSync::SyncType::TYPE_BACKUP)
            {
                message = localFolders.value(existingPath) == MegaSync::SyncType::TYPE_TWOWAY ?
                            tr("You can't backup this folder as it's already synced.")
                          : tr("Folder is already backed up. Select a different one.");
            }
            else
            {
                message = localFolders.value(existingPath) == MegaSync::SyncType::TYPE_TWOWAY ?
                            tr("You can't sync this folder as it's already synced.")
                          : tr("You can't sync this folder as it's already backed up.");
            }
        }
        else if (inputPath.startsWith(existingPath)
                 && inputPath[existingPath.size() - QDir(existingPath).isRoot()] == QDir::separator())
        {
            if (syncType == MegaSync::SyncType::TYPE_BACKUP)
            {
                message = localFolders.value(existingPath) == MegaSync::SyncType::TYPE_TWOWAY ?
                            tr("You can't backup this folder as it's already inside a synced folder.")
                          : getErrStrCurrentBackupInsideExistingBackup();
            }
            else
            {
                message = localFolders.value(existingPath) == MegaSync::SyncType::TYPE_TWOWAY ?
                            tr("You can't sync folders that are inside synced folders.")
                          : tr("You can't sync folders that are inside backed up folders.");
            }
        }
        else if (existingPath.startsWith(inputPath)
                 && existingPath[inputPath.size() - QDir(inputPath).isRoot()] == QDir::separator())
        {
            if (syncType == MegaSync::SyncType::TYPE_BACKUP)
            {
                message = localFolders.value(existingPath) == MegaSync::SyncType::TYPE_TWOWAY ?
                            tr("You can't backup this folder as it contains synced folders.")
                          : getErrStrCurrentBackupOverExistingBackup();
            }
            else
            {
                message = localFolders.value(existingPath) == MegaSync::SyncType::TYPE_TWOWAY ?
                            tr("You can't sync folders that contain synced folders.")
                          : tr("You can't sync folders that contain backed up folders.");
            }
        }
    }
    return message;
}

SyncController::Syncability SyncController::isLocalFolderAlreadySynced(const QString& path, const MegaSync::SyncType &syncType, QString& message)
{
    message = getIsLocalFolderAlreadySyncedMsg(path, syncType);
    return (message.isEmpty() ? Syncability::CAN_SYNC : Syncability::CANT_SYNC);
}

QString SyncController::getIsLocalFolderAllowedForSyncMsg(const QString& path, const MegaSync::SyncType& syncType)
{
    QString inputPath (path);
    QString message;

#ifdef WIN32
    if (inputPath.startsWith(QString::fromLatin1("\\\\?\\")))
    {
        inputPath = inputPath.mid(4);
    }
#endif

    // Use canonicalPath() to resolve links
    inputPath = QDir(inputPath).canonicalPath();

    if (inputPath == QDir::rootPath())
    {
        if (syncType == MegaSync::SyncType::TYPE_BACKUP)
        {
            message = tr("You can’t backup “%1” as it’s the root folder. "
                         "The root folder is either; the top-level folder "
                         "on your device or computer that holds all your "
                         "folders and files or the folder where the system "
                         "or program is installed. We don’t allow users to "
                         "back up root folders as it may cause file conflicts or errors.\n"
                         "To continue, select a different folder.").arg(inputPath);
        }
        else
        {
            message = tr("You can’t sync “%1” as it’s the root folder. "
                         "The root folder is either; the top-level folder on your "
                         "device or computer that holds all your folders and files"
                         " or the folder where the system or program is installed."
                         " We don’t allow users to sync root folders as it may cause"
                         " file conflicts or errors.\n"
                         "To continue, select a different folder.").arg(inputPath);
        }
    }
    return message;
}

SyncController::Syncability SyncController::isLocalFolderAllowedForSync(const QString& path, const MegaSync::SyncType &syncType, QString& message)
{
    message = getIsLocalFolderAllowedForSyncMsg(path, syncType);
    return (message.isEmpty() ? Syncability::CAN_SYNC : Syncability::CANT_SYNC);
}

QString SyncController::getAreLocalFolderAccessRightsOkMsg(const QString& path, const mega::MegaSync::SyncType& syncType)
{
    QString message;

    // We only check rw rights for two-way syncs
    if (syncType == MegaSync::TYPE_TWOWAY)
    {
        QTemporaryFile test (path + QDir::separator());
        if (!QDir(path).mkpath(QString::fromLatin1(".")) && !test.open())
        {
            message = tr("You don't have write permissions in this local folder.")
                    + QChar::fromLatin1('\n')
                    + tr("MEGAsync won't be able to download anything here.");
        }
    }
    return message;
}

SyncController::Syncability SyncController::areLocalFolderAccessRightsOk(const QString& path, const mega::MegaSync::SyncType& syncType, QString& message)
{
    message = getAreLocalFolderAccessRightsOkMsg(path, syncType);
    return (message.isEmpty() ? Syncability::CAN_SYNC : Syncability::WARN_SYNC);
}

// Returns wether the path is syncable.
// The message to display to the user is stored in <message>.
// The first error encountered is returned.
// Errors trump warnings
// In case of several warnings, only the last one is returned.
SyncController::Syncability SyncController::isLocalFolderSyncable(const QString& path, const mega::MegaSync::SyncType& syncType, QString& message)
{
    Syncability syncability (Syncability::CAN_SYNC);

    // First check if the path is allowed
    syncability = isLocalFolderAllowedForSync(path, syncType, message);

    // The check if it is not synced already
    if (syncability != Syncability::CANT_SYNC)
    {
        syncability = std::max(isLocalFolderAlreadySynced(path, syncType, message), syncability);
    }

    // Then check that we have rw rights for this path
    if (syncability != Syncability::CANT_SYNC)
    {
        syncability = std::max(areLocalFolderAccessRightsOk(path, syncType, message), syncability);
    }

    return (syncability);
}

// Returns wether the remote path is syncable.
// The message to display to the user is stored in <message>.
SyncController::Syncability SyncController::isRemoteFolderSyncable(std::shared_ptr<mega::MegaNode> node, QString& message)
{
    Syncability syncability (Syncability::CANT_SYNC);
    std::unique_ptr<MegaError> err (MegaSyncApp->getMegaApi()->isNodeSyncableWithError(node.get()));
    switch (err->getErrorCode())
    {
    case MegaError::API_OK:
    {
        syncability = Syncability::CAN_SYNC;
        break;
    }
    case MegaError::API_EACCESS:
    {
        switch (err->getSyncError())
        {
        case SyncError::SHARE_NON_FULL_ACCESS:
        {
            message = tr("You don't have enough permissions for this remote folder.");
            break;
        }
        case SyncError::REMOTE_NODE_INSIDE_RUBBISH:
        case SyncError::INVALID_REMOTE_TYPE:
        {
            message = tr("Invalid remote path.");
            break;
        }
        }
        break;
    }
    case MegaError::API_EEXIST:
    {
        switch (err->getSyncError())
        {
        case SyncError::ACTIVE_SYNC_SAME_PATH:
        {
            message = tr("The selected MEGA folder is already synced.");
            break;
        }
        case SyncError::ACTIVE_SYNC_BELOW_PATH:
        {
            message = tr("Folder contents already synced.");
            break;
        }
        case SyncError::ACTIVE_SYNC_ABOVE_PATH:
        {
            message = tr("Folder already synced.");
            break;
        }
        }
        break;
    }
    case MegaError::API_ENOENT:
    case MegaError::API_EARGS:
    default:
    {
        message = tr("Invalid remote path.");
        break;
    }
    }
    return (syncability);
}

QString SyncController::getSyncNameFromPath(const QString& path)
{
    QDir dir (path);
    QString syncName;

    // Handle fs root case
    if (dir.isRoot())
    {
        // Cleanup the path (in Windows: get "F:" from "F:\")
        QString cleanPath (QDir::toNativeSeparators(dir.absolutePath()).remove(QDir::separator()));
        // Try to get the volume label
        QStorageInfo storage(dir);
        QString label (QString::fromUtf8(storage.subvolume()));
        if (label.isEmpty())
        {
            label = storage.name();
        }
        // If we have no label, fallback to the cleaned path
        syncName = label.isEmpty() ? cleanPath
                                   : QString::fromUtf8("%1 (%2)").arg(label, cleanPath);
    }
    else
    {
        // Take the folder name as sync name
        syncName = dir.dirName();
    }

    return syncName;
}

QString SyncController::getErrStrCurrentBackupOverExistingBackup()
{
    return tr("You can't backup this folder as it contains backed up folders.");
}

QString SyncController::getErrStrCurrentBackupInsideExistingBackup()
{
    return tr("You can't backup this folder as it's already inside a backed up folder.");
}

QString SyncController::getSyncAPIErrorMsg(int megaError)
{
    switch (megaError)
    {
        case MegaError::API_EARGS:
            return tr("Unable to create backup as selected folder is not valid. Try again.");
        break;
        case MegaError::API_EACCESS:
            return tr("Unable to create backup. Try again and if issue continues, contact [A]Support[/A].");
        break;
        case MegaError::API_EINCOMPLETE:
            return tr("Unable to create backup as the device you're backing up from doesn't have a name. "
                     "Give your device a name and then try again. If issue continues, contact [A]Support[/A].");
        case MegaError::API_EINTERNAL:
        // Fallthrough
        case MegaError::API_ENOENT:
        // Fallthrough
        case MegaError::API_EEXIST:
            return tr("Unable to create backup. For further information, contact [A]Support[/A].");
        default:
            break;
    }
    return QString();
}

QString SyncController::getSyncTypeString(const mega::MegaSync::SyncType& syncType)
{
    QString typeString;
    switch (syncType)
    {
        case MegaSync::SyncType::TYPE_TWOWAY:
        {
            typeString = QLatin1String("Two-way");
            break;
        }
        case MegaSync::SyncType::TYPE_BACKUP:
        {
            typeString = QLatin1String("Backup");
            break;
        }
        case MegaSync::SyncType::TYPE_UNKNOWN:
        default:
        {
            typeString = QLatin1String("Unknown");
            break;
        }
    }
    return typeString;
}

void SyncController::onRequestFinish(MegaApi *api, MegaRequest *req, MegaError *e)
{
    int errorCode (e->getErrorCode());

    switch(req->getType())
    {
        case MegaRequest::TYPE_ADD_SYNC:
        {
            int syncErrorCode (req->getNumDetails());
            QString errorMsg;

            bool error = false;

            if (syncErrorCode != MegaSync::NO_SYNC_ERROR)
            {
                errorMsg = QCoreApplication::translate("MegaSyncError", MegaSync::getMegaSyncErrorCode(syncErrorCode));
                error = true;
            }
            else if (errorCode != MegaError::API_OK)
            {
                errorMsg = getSyncAPIErrorMsg(errorCode);
                if(errorMsg.isEmpty())
                    errorMsg = QCoreApplication::translate("MegaError", e->getErrorString());
                error = true;
            }

            if(error)
            {
                std::shared_ptr<MegaNode> remoteNode(api->getNodeByHandle(req->getNodeHandle()));
                QString logMsg = QString::fromUtf8("Error adding sync (%1) \"%2\" for \"%3\" to \"%4\" (request error): %5").arg(
                                 getSyncTypeString(static_cast<MegaSync::SyncType>(req->getParamType())),
                                 QString::fromUtf8(req->getName()),
                                 QString::fromUtf8(req->getFile()),
                                 QString::fromUtf8(api->getNodePath(remoteNode.get())),
                                 errorMsg);
                MegaApi::log(MegaApi::LOG_LEVEL_ERROR, logMsg.toUtf8().constData());
            }
            emit syncAddStatus(errorCode, errorMsg, QString::fromUtf8(req->getFile()));
            break;
        }
    }
}
