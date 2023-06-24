DEPENDPATH += $$PWD
INCLUDEPATH += $$PWD
INCLUDEPATH += $$PWD/model
INCLUDEPATH += $$PWD/gui

SOURCES += $$PWD/model/TransfersModel.cpp \
           $$PWD/gui/DuplicatedNodeDialogs/DuplicatedNodeDialog.cpp \
           $$PWD/gui/DuplicatedNodeDialogs/DuplicatedNodeInfo.cpp \
           $$PWD/gui/DuplicatedNodeDialogs/DuplicatedNodeItem.cpp \
           $$PWD/gui/DuplicatedNodeDialogs/DuplicatedUploadChecker.cpp \
           $$PWD/gui/InfoDialogTransferLoadingItem.cpp \
           $$PWD/model/InfoDialogTransfersProxyModel.cpp \
           $$PWD/model/TransfersManagerSortFilterProxyModel.cpp \
           $$PWD/model/TransferMetaData.cpp \
           $$PWD/gui/InfoDialogTransferDelegateWidget.cpp \
           $$PWD/gui/InfoDialogTransfersWidget.cpp \
           $$PWD/gui/MegaTransferDelegate.cpp  \
           $$PWD/gui/MegaTransferView.cpp \
           $$PWD/gui/TransferBaseDelegateWidget.cpp \
           $$PWD/gui/TransferItem.cpp \
           $$PWD/gui/TransferManager.cpp \
           $$PWD/gui/TransferManagerDelegateWidget.cpp \
           $$PWD/gui/TransferManagerLoadingItem.cpp \
           $$PWD/gui/TransfersStatusWidget.cpp \
           $$PWD/gui/TransfersSummaryWidget.cpp \
           $$PWD/gui/TransferScanCancelUi.cpp \
           $$PWD/gui/TransferWidgetHeaderItem.cpp \
           $$PWD/gui/TransfersWidget.cpp

HEADERS += $$PWD/model/InfoDialogTransfersProxyModel.h \
           $$PWD/gui/DuplicatedNodeDialogs/DuplicatedNodeDialog.h \
           $$PWD/gui/DuplicatedNodeDialogs/DuplicatedNodeInfo.h \
           $$PWD/gui/DuplicatedNodeDialogs/DuplicatedNodeItem.h \
           $$PWD/gui/DuplicatedNodeDialogs/DuplicatedUploadChecker.h \
           $$PWD/gui/InfoDialogTransferLoadingItem.h \
           $$PWD/model/TransfersManagerSortFilterProxyModel.h \
           $$PWD/model/TransfersSortFilterProxyBaseModel.h \
           $$PWD/model/TransfersModel.h \
           $$PWD/model/TransferMetaData.h \
           $$PWD/gui/InfoDialogTransferDelegateWidget.h \
           $$PWD/gui/InfoDialogTransfersWidget.h \
           $$PWD/gui/MegaTransferDelegate.h  \
           $$PWD/gui/MegaTransferView.h \
           $$PWD/gui/TransferBaseDelegateWidget.h \
           $$PWD/gui/TransferItem.h \
           $$PWD/gui/TransferManager.h \
           $$PWD/gui/TransferManagerDelegateWidget.h \
           $$PWD/gui/TransferManagerLoadingItem.h \
           $$PWD/gui/TransfersStatusWidget.h \
           $$PWD/gui/TransfersSummaryWidget.h \
           $$PWD/gui/TransferScanCancelUi.h \
           $$PWD/gui/TransferWidgetHeaderItem.h \
           $$PWD/gui/TransfersWidget.h

win32 {
    RESOURCES += $$PWD/../gui/Resources_win.qrc
    INCLUDEPATH += $$PWD/win
    FORMS    += $$PWD/gui/win/TransferWidgetHeaderItem.ui \
                $$PWD/gui/win/TransferManagerDelegateWidget.ui \
                $$PWD/gui/win/TransferManager.ui \
                $$PWD/gui/win/TransfersWidget.ui \
                $$PWD/gui/win/TransferManagerLoadingItem.ui \
                $$PWD/gui/win/TransferManagerDragBackDrop.ui \
                $$PWD/gui/win/InfoDialogTransfersWidget.ui \
                $$PWD/gui/win/InfoDialogTransferDelegateWidget.ui \
                $$PWD/gui/win/InfoDialogTransferLoadingItem.ui \
                $$PWD/gui/win/TransfersStatusWidget.ui \
                $$PWD/gui/win/TransfersSummaryWidget.ui \
                $$PWD/gui/DuplicatedNodeDialogs/win/DuplicatedNodeDialog.ui \
                $$PWD/gui/DuplicatedNodeDialogs/win/DuplicatedNodeItem.ui
}

macx {
    RESOURCES += $$PWD/../gui/Resources_macx.qrc
    INCLUDEPATH += $$PWD/macx
    FORMS    += $$PWD/gui/macx/TransferWidgetHeaderItem.ui \
                $$PWD/gui/macx/TransferManagerDelegateWidget.ui \
                $$PWD/gui/macx/TransferManager.ui \
                $$PWD/gui/macx/TransfersWidget.ui \
                $$PWD/gui/macx/TransferManagerLoadingItem.ui \
                $$PWD/gui/macx/TransferManagerDragBackDrop.ui \
                $$PWD/gui/macx/InfoDialogTransfersWidget.ui \
                $$PWD/gui/macx/InfoDialogTransferDelegateWidget.ui \
                $$PWD/gui/macx/InfoDialogTransferLoadingItem.ui \
                $$PWD/gui/macx/TransfersStatusWidget.ui \
                $$PWD/gui/macx/TransfersSummaryWidget.ui \
                $$PWD/gui/DuplicatedNodeDialogs/macx/DuplicatedNodeDialog.ui \
                $$PWD/gui/DuplicatedNodeDialogs/macx/DuplicatedNodeItem.ui
}

unix:!macx {
    RESOURCES += $$PWD/../gui/Resources_linux.qrc
    INCLUDEPATH += $$PWD/linux
    FORMS    += $$PWD/gui/linux/TransferWidgetHeaderItem.ui \
                $$PWD/gui/linux/TransferManagerDelegateWidget.ui \
                $$PWD/gui/linux/TransferManager.ui \
                $$PWD/gui/linux/TransfersWidget.ui \
                $$PWD/gui/linux/TransferManagerLoadingItem.ui \
                $$PWD/gui/linux/TransferManagerDragBackDrop.ui \
                $$PWD/gui/linux/InfoDialogTransfersWidget.ui \
                $$PWD/gui/linux/InfoDialogTransferDelegateWidget.ui \
                $$PWD/gui/linux/InfoDialogTransferLoadingItem.ui \
                $$PWD/gui/linux/TransfersStatusWidget.ui \
                $$PWD/gui/linux/TransfersSummaryWidget.ui \
                $$PWD/gui/DuplicatedNodeDialogs/linux/DuplicatedNodeDialog.ui \
                $$PWD/gui/DuplicatedNodeDialogs/linux/DuplicatedNodeItem.ui
}
