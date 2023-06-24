INCLUDEPATH += $$PWD/model
INCLUDEPATH += $$PWD/gui

SOURCES += $$PWD/gui/Backups/AddBackupDialog.cpp \
           $$PWD/gui/Backups/BackupNameConflictDialog.cpp \
           $$PWD/gui/Backups/BackupRenameWidget.cpp \
           $$PWD/gui/Backups/BackupTableView.cpp \
           $$PWD/gui/Backups/BackupTableViewTooltips.cpp \
           $$PWD/gui/Backups/BackupsWizard.cpp \
           $$PWD/gui/Backups/RemoveBackupDialog.cpp \
           $$PWD/gui/SyncTooltipCreator.cpp \
           $$PWD/gui/SyncsMenu.cpp \
           $$PWD/gui/Twoways/BindFolderDialog.cpp \
           $$PWD/gui/Twoways/FolderBinder.cpp \
           $$PWD/gui/Twoways/SyncTableView.cpp \
           $$PWD/gui/Twoways/SyncTableViewTooltips.cpp \
           $$PWD/model/BackupItemModel.cpp \
           $$PWD/model/SyncItemModel.cpp \
           $$PWD/control/SyncInfo.cpp \
           $$PWD/control/SyncController.cpp \
           $$PWD/control/SyncSettings.cpp

HEADERS += $$PWD/gui/Backups/AddBackupDialog.h \
           $$PWD/gui/Backups/BackupNameConflictDialog.h \
           $$PWD/gui/Backups/BackupRenameWidget.h \
           $$PWD/gui/Backups/BackupTableView.h \
           $$PWD/gui/Backups/BackupTableViewTooltips.h \
           $$PWD/gui/Backups/BackupsWizard.h \
           $$PWD/gui/Backups/RemoveBackupDialog.h \
           $$PWD/gui/SyncTooltipCreator.h \
           $$PWD/gui/SyncsMenu.h \
           $$PWD/gui/Twoways/BindFolderDialog.h \
           $$PWD/gui/Twoways/FolderBinder.h \
           $$PWD/gui/Twoways/SyncTableView.h \
           $$PWD/gui/Twoways/SyncTableViewTooltips.h \
           $$PWD/model/BackupItemModel.h \
           $$PWD/model/SyncItemModel.h \
           $$PWD/control/SyncController.h \
           $$PWD/control/SyncInfo.h \
           $$PWD/control/SyncSettings.h

win32 {
    INCLUDEPATH += $$PWD/win
    RESOURCES += $$PWD/../gui/Resources_win.qrc
    FORMS    += $$PWD/gui/Twoways/win/FolderBinder.ui \
                $$PWD/gui/Twoways/win/BindFolderDialog.ui \
                $$PWD/gui/Backups/win/BackupsWizard.ui \
                $$PWD/gui/Backups/win/AddBackupDialog.ui \
                $$PWD/gui/Backups/win/RemoveBackupDialog.ui \
                $$PWD/gui/Backups/win/BackupNameConflictDialog.ui \
                $$PWD/gui/Backups/win/BackupRenameWidget.ui
}

macx {
    INCLUDEPATH += $$PWD/macx
    RESOURCES += $$PWD/../gui/Resources_macx.qrc
    FORMS    += $$PWD/gui/Twoways/macx/FolderBinder.ui \
                $$PWD/gui/Twoways/macx/BindFolderDialog.ui \
                $$PWD/gui/Backups/macx/BackupsWizard.ui \
                $$PWD/gui/Backups/macx/AddBackupDialog.ui \
                $$PWD/gui/Backups/macx/RemoveBackupDialog.ui \
                $$PWD/gui/Backups/macx/BackupNameConflictDialog.ui \
                $$PWD/gui/Backups/macx/BackupRenameWidget.ui
}

unix:!macx {
    INCLUDEPATH += $$PWD/linux
    RESOURCES += $$PWD/../gui/Resources_linux.qrc
    FORMS    += $$PWD/gui/Twoways/linux/FolderBinder.ui \
                $$PWD/gui/Twoways/linux/BindFolderDialog.ui \
                $$PWD/gui/Backups/linux/BackupsWizard.ui \
                $$PWD/gui/Backups/linux/AddBackupDialog.ui \
                $$PWD/gui/Backups/linux/RemoveBackupDialog.ui \
                $$PWD/gui/Backups/linux/BackupNameConflictDialog.ui \
                $$PWD/gui/Backups/linux/BackupRenameWidget.ui
}
