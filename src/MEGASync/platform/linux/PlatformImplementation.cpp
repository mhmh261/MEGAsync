#include "PlatformImplementation.h"

#include <QSet>
#include <QX11Info>

#include <cstdlib>
#include <cstring>
#include <map>

using namespace std;
using namespace mega;

PlatformImplementation::PlatformImplementation()
{
    autostart_dir = QDir::homePath() + QString::fromAscii("/.config/autostart/");
    desktop_file = autostart_dir + QString::fromAscii("megasync.desktop");
    set_icon = QString::fromUtf8("gvfs-set-attribute -t string \"%1\" metadata::custom-icon file://%2");
    remove_icon = QString::fromUtf8("gvfs-set-attribute -t unset \"%1\" metadata::custom-icon");
    custom_icon = QString::fromUtf8("/usr/share/icons/hicolor/256x256/apps/mega.png");
}

void PlatformImplementation::initialize(int /*argc*/, char** /*argv*/)
{
    mShellNotifier = std::make_shared<SignalShellNotifier>();
}

void PlatformImplementation::notifyItemChange(const QString& path, int)
{
    if (!path.isEmpty())
    {
        if (notify_server && !Preferences::instance()->overlayIconsDisabled())
        {
            std::string stdPath = path.toStdString();
            notify_server->notifyItemChange(&stdPath);
        }
        mShellNotifier->notify(path);
    }
}

void PlatformImplementation::notifySyncFileChange(std::string *localPath, int newState)
{
    if(localPath && localPath->size())
    {
        notifyItemChange(QString::fromStdString(*localPath), newState);
    }
}

// enable or disable MEGASync launching at startup
// return true if operation succeeded
bool PlatformImplementation::startOnStartup(bool value)
{
    // copy desktop file into autostart directory
    if (value)
    {
        if (QFile(desktop_file).exists())
        {
            return true;
        }
        else
        {
            // make sure directory exist
            if (!QDir(autostart_dir).exists())
            {
                if (!QDir().mkdir(autostart_dir))
                {
                    //LOG_debug << "Failed to create autostart dir: " << autostart_dir;
                    return false;
                }
            }
            QString app_desktop = QString::fromAscii("/usr/share/applications/megasync.desktop");
            if (QFile(app_desktop).exists())
            {
                return QFile::copy(app_desktop, desktop_file);
            }
            else
            {
                //LOG_debug << "Desktop file does not exist: " << app_desktop;
                return false;
            }
        }
    }
    else
    {
        // remove desktop file if it exists
        if (QFile(desktop_file).exists())
        {
            return QFile::remove(desktop_file);
        }
    }
    return true;
}

bool PlatformImplementation::isStartOnStartupActive()
{
    return QFile(desktop_file).exists();
}

bool PlatformImplementation::isTilingWindowManager()
{
    static const QSet<QString> tiling_wms = {
        QString::fromUtf8("i3")
    };

    return getValue("MEGASYNC_ASSUME_TILING_WM", false)
           || tiling_wms.contains(getWindowManagerName());
}

bool PlatformImplementation::showInFolder(QString pathIn)
{
    QString filebrowser = getDefaultFileBrowserApp();
    // Nautilus on Gnome, does not open the directory if argument is given without surrounding double-quotes;
    // Path is passed through QUrl which properly escapes special chars in native platform URIs
    // which takes care of path names also containing double-quotes withing, which will stop
    // Nautilus from parsing the argument string all-together
    return QProcess::startDetached(filebrowser + QString::fromLatin1(" \"")
                            + QUrl::fromLocalFile(pathIn).toString() + QString::fromLatin1("\""));
}

void PlatformImplementation::startShellDispatcher(MegaApplication *receiver)
{
    if (!ext_server)
    {
        ext_server = new ExtServer(receiver);
    }

    if (!notify_server)
    {
        notify_server = new NotifyServer();
    }
}

void PlatformImplementation::stopShellDispatcher()
{
    if (ext_server)
    {
        delete ext_server;
        ext_server = NULL;
    }

    if (notify_server)
    {
        delete notify_server;
        notify_server = NULL;
    }
}

void PlatformImplementation::syncFolderAdded(QString syncPath, QString /*syncName*/, QString /*syncID*/)
{
    if (QFile(custom_icon).exists())
    {
        QFile *folder = new QFile(syncPath);
        if (folder->exists())
        {
            QProcess::startDetached(set_icon.arg(folder->fileName()).arg(custom_icon));
        }
        delete folder;

    }

    if (notify_server)
    {
        notify_server->notifySyncAdd(syncPath);
    }
}

void PlatformImplementation::syncFolderRemoved(QString syncPath, QString /*syncName*/, QString /*syncID*/)
{
    QFile *folder = new QFile(syncPath);
    if (folder->exists())
    {
        QProcess::startDetached(remove_icon.arg(folder->fileName()));
    }
    delete folder;

    if (notify_server)
    {
        notify_server->notifySyncDel(syncPath);
    }
}

void PlatformImplementation::notifyRestartSyncFolders()
{

}

void PlatformImplementation::notifyAllSyncFoldersAdded()
{

}

void PlatformImplementation::notifyAllSyncFoldersRemoved()
{

}

QString PlatformImplementation::getDefaultFileBrowserApp()
{
    return getDefaultOpenAppByMimeType(QString::fromUtf8("inode/directory"));
}

QString PlatformImplementation::getDefaultOpenApp(QString extension)
{
    char *mimeType = MegaApi::getMimeType(extension.toUtf8().constData());
    if (!mimeType)
    {
        return QString();
    }
    QString qsMimeType(QString::fromUtf8(mimeType));
    delete mimeType;
    return getDefaultOpenAppByMimeType(qsMimeType);
}

QString PlatformImplementation::getDefaultOpenAppByMimeType(QString mimeType)
{
    QString getDefaultAppDesktopFileName = QString::fromUtf8("xdg-mime query default ") + mimeType;

    QProcess process;
    process.start(getDefaultAppDesktopFileName,
                  QIODevice::ReadWrite | QIODevice::Text);
    if(!process.waitForFinished(5000))
    {
        return QString();
    }

    QString desktopFileName = QString::fromUtf8(process.readAllStandardOutput());
    desktopFileName = desktopFileName.trimmed();
    desktopFileName.replace(QString::fromUtf8(";"), QString::fromUtf8(""));
    if (!desktopFileName.size())
    {
        return QString();
    }

    QFileInfo desktopFile(QString::fromUtf8("/usr/share/applications/") + desktopFileName);
    if (!desktopFile.exists())
    {
        return QString();
    }

    QFile f(desktopFile.absoluteFilePath());
    if (!f.open(QFile::ReadOnly | QFile::Text))
    {
        return QString();
    }

    QTextStream in(&f);
    QStringList contents = in.readAll().split(QString::fromUtf8("\n"));
    contents = contents.filter(QRegExp(QString::fromUtf8("^Exec=")));
    if (!contents.size())
    {
        return QString();
    }

    QString line = contents.first();
    int index = line.indexOf(QChar::fromAscii('%'));
    int size = -1;
    if (index != -1)
    {
        size = index - 6;
    }

    if (!size)
    {
        return QString();
    }

    return line.mid(5, size);
}

bool PlatformImplementation::getValue(const char * const name, const bool default_value)
{
    const char * const value = getenv(name);

    if (!value)
        return default_value;

    return strcmp(value, "0") != 0;
}

std::string PlatformImplementation::getValue(const char * const name, const std::string &default_value)
{
    const char * const value = getenv(name);

    if (!value)
        return default_value;

    return value;
}

QString PlatformImplementation::getWindowManagerName()
{
    static QString wmName;
    static bool cached = false;

    if (!cached)
    {
        const int maxLen = 1024;
        const auto connection = QX11Info::connection();
        const auto appRootWindow = static_cast<xcb_window_t>(QX11Info::appRootWindow());

        auto wmCheckAtom = getAtom(QX11Info::connection(), "_NET_SUPPORTING_WM_CHECK");
        // Get window manager
        auto reply = xcb_get_property_reply(connection,
                                            xcb_get_property(connection,
                                                             false,
                                                             appRootWindow,
                                                             wmCheckAtom,
                                                             XCB_ATOM_WINDOW,
                                                             0,
                                                             maxLen),
                                            nullptr);

        if (reply && reply->format == 32 && reply->type == XCB_ATOM_WINDOW)
        {
            // Get window manager name
            const xcb_window_t windowManager = *(static_cast<xcb_window_t*>(xcb_get_property_value(reply)));

            if (windowManager != XCB_WINDOW_NONE)
            {
                const auto utf8StringAtom = getAtom(connection, "UTF8_STRING");
                const auto wmNameAtom = getAtom(connection, "_NET_WM_NAME");

                auto wmReply = xcb_get_property_reply(connection,
                                                      xcb_get_property(connection,
                                                                       false,
                                                                       windowManager,
                                                                       wmNameAtom,
                                                                       utf8StringAtom,
                                                                       0,
                                                                       maxLen),
                                                      nullptr);
                if (wmReply && wmReply->format == 8 && wmReply->type == utf8StringAtom)
                {
                    wmName = QString::fromUtf8(static_cast<const char*>(xcb_get_property_value(wmReply)),
                                                                        xcb_get_property_value_length(wmReply));
                }
                free(wmReply);
            }
        }
        free(reply);
        cached = true;
    }
    return wmName;
}

bool PlatformImplementation::registerUpdateJob()
{
    return true;
}

// Check if it's needed to start the local HTTP server
// for communications with the webclient
bool PlatformImplementation::shouldRunHttpServer()
{
    QStringList data = getListRunningProcesses();
    if (data.size() > 1)
    {
        for (int i = 1; i < data.size(); i++)
        {
            // The MEGA webclient sends request to MEGAsync to improve the
            // user experience. We check if web browsers are running because
            // otherwise it isn't needed to run the local web server for this purpose.
            // Here is the list or web browsers that allow HTTP communications
            // with 127.0.0.1 inside HTTPS webs.
            QString command = data.at(i).trimmed();
            if (command.contains(QString::fromUtf8("firefox"), Qt::CaseInsensitive)
                    || command.contains(QString::fromUtf8("chrome"), Qt::CaseInsensitive)
                    || command.contains(QString::fromUtf8("chromium"), Qt::CaseInsensitive)
                    )
            {
                return true;
            }
        }
    }
    return false;
}

// Check if it's needed to start the local HTTPS server
// for communications with the webclient
bool PlatformImplementation::shouldRunHttpsServer()
{
    QStringList data = getListRunningProcesses();

    if (data.size() > 1)
    {
        for (int i = 1; i < data.size(); i++)
        {
            // The MEGA webclient sends request to MEGAsync to improve the
            // user experience. We check if web browsers are running because
            // otherwise it isn't needed to run the local web server for this purpose.
            // Here is the list or web browsers that don't allow HTTP communications
            // with 127.0.0.1 inside HTTPS webs and therefore require a HTTPS server.
            QString command = data.at(i).trimmed();
            if (command.contains(QString::fromUtf8("safari"), Qt::CaseInsensitive)
                    || command.contains(QString::fromUtf8("iexplore"), Qt::CaseInsensitive)
                    || command.contains(QString::fromUtf8("opera"), Qt::CaseInsensitive)
                    || command.contains(QString::fromUtf8("iceweasel"), Qt::CaseInsensitive)
                    || command.contains(QString::fromUtf8("konqueror"), Qt::CaseInsensitive)
                    )
            {
                return true;
            }
        }
    }
    return false;
}

bool PlatformImplementation::isUserActive()
{
    return true;
}

QString PlatformImplementation::getDeviceName()
{
    // First, try to read maker and model
    QString vendor;
    QFile vendorFile(QLatin1Literal("/sys/devices/virtual/dmi/id/board_vendor"));
    if (vendorFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
            vendor = QString::fromUtf8(vendorFile.readLine()).trimmed();
    }
    vendorFile.close();

    QString model;
    QFile modelFile(QLatin1Literal("/sys/devices/virtual/dmi/id/product_name"));
    if (modelFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
            model = QString::fromUtf8(modelFile.readLine()).trimmed();
    }
    modelFile.close();

    QString deviceName;
    // If failure or empty strings, give hostname
    if (vendor.isEmpty() && model.isEmpty())
    {
        deviceName = QSysInfo::machineHostName();
        deviceName.remove(QLatin1Literal(".local"));
    }
    else
    {
        deviceName = vendor + QLatin1Literal(" ") + model;
    }

    return deviceName;
}

void PlatformImplementation::fileSelector(QString title, QString defaultDir, bool multiSelection, QWidget *parent, std::function<void(QStringList)> func)
{
    if (defaultDir.isEmpty())
    {
        defaultDir = QLatin1String("/");
    }
    AbstractPlatform::fileSelector(title, defaultDir, multiSelection, parent, func);
}

void PlatformImplementation::folderSelector(QString title, QString defaultDir, bool multiSelection, QWidget *parent, std::function<void(QStringList)> func)
{
    if (defaultDir.isEmpty())
    {
        defaultDir = QLatin1String("/");
    }
    AbstractPlatform::folderSelector(title, defaultDir, multiSelection, parent, func);
}
void PlatformImplementation::fileAndFolderSelector(QString title, QString defaultDir, bool multiSelection, QWidget *parent, std::function<void(QStringList)> func)
{
    if (defaultDir.isEmpty())
    {
        defaultDir = QLatin1String("/");
    }
    AbstractPlatform::fileAndFolderSelector(title, defaultDir, multiSelection, parent, func);
}

QStringList PlatformImplementation::getListRunningProcesses()
{
    QProcess p;
    p.start(QString::fromUtf8("ps ax -o comm"));
    p.waitForFinished(2000);
    QString output = QString::fromUtf8(p.readAllStandardOutput().constData());
    QString e = QString::fromUtf8(p.readAllStandardError().constData());
    if (e.size())
    {
        MegaApi::log(MegaApi::LOG_LEVEL_ERROR, "Error for \"ps ax -o comm\" command:");
        MegaApi::log(MegaApi::LOG_LEVEL_ERROR, e.toUtf8().constData());
    }

    p.start(QString::fromUtf8("/bin/bash -c \"readlink /proc/*/exe\""));
    p.waitForFinished(2000);
    QString output2 = QString::fromUtf8(p.readAllStandardOutput().constData());
    e = QString::fromUtf8(p.readAllStandardError().constData());
    if (e.size())
    {
        MegaApi::log(MegaApi::LOG_LEVEL_ERROR, "Error for \"readlink /proc/*/exe\" command:");
        MegaApi::log(MegaApi::LOG_LEVEL_ERROR, e.toUtf8().constData());
    }
    QStringList data = output.split(QString::fromUtf8("\n"));

    data.append(output2.split(QString::fromUtf8("\n")));

    return data;
}

xcb_atom_t PlatformImplementation::getAtom(xcb_connection_t * const connection, const char *name)
{
    xcb_intern_atom_cookie_t cookie =
      xcb_intern_atom(connection, 0, static_cast<uint16_t>(strlen(name)), name);
    xcb_intern_atom_reply_t *reply =
      xcb_intern_atom_reply(connection, cookie, nullptr);

    if (!reply)
        return XCB_ATOM_NONE;

    xcb_atom_t result = reply->atom;
    free(reply);

    return result;
}
