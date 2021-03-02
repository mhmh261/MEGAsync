#include "TransfersSortFilterProxyModel.h"
#include <megaapi.h>

TransfersSortFilterProxyModel::TransfersSortFilterProxyModel(QObject* parent)
    : QSortFilterProxyModel(parent),
      mTransferType(QSet<int>()),
      mTransferState(QSet<int>()),
      mFileType(QSet<TransferData::FileTypes>())
{
}

void TransfersSortFilterProxyModel::setTransferType(const QSet<int> transferTypes)
{
    mTransferType = transferTypes;
    invalidateFilter();
}

void TransfersSortFilterProxyModel::addTransferType(const QSet<int> transferTypes)
{
    mTransferType += transferTypes;
    invalidateFilter();
}

void TransfersSortFilterProxyModel::setTransferState(const QSet<int> transferStates)
{
    mTransferState = transferStates;
    invalidateFilter();
}

void TransfersSortFilterProxyModel::addTransferState(const QSet<int> transferStates)
{
    mTransferState += transferStates;
    invalidateFilter();
}

void TransfersSortFilterProxyModel::setFileType(const QSet<TransferData::FileTypes> fileTypes)
{
    mFileType = fileTypes;
    invalidateFilter();
}

void TransfersSortFilterProxyModel::addFileType(const QSet<TransferData::FileTypes> fileTypes)
{
    mFileType += fileTypes;
    invalidateFilter();
}

bool TransfersSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

    const auto transferItem (qvariant_cast<TransferItem2>(index.data()));

    return (mTransferState.isEmpty() || mTransferState.contains(transferItem.getState()))
            && (mTransferType.isEmpty() || mTransferType.contains(transferItem.getType()))
            && (mFileType.isEmpty() || mFileType.contains(transferItem.getFileType()))
            && transferItem.getTransferData()->mFilename.contains(filterRegExp());
}

bool TransfersSortFilterProxyModel::lessThan(const QModelIndex &left, const QModelIndex &right) const
{
    const auto leftItem (qvariant_cast<TransferItem2>(left.data()));
    const auto rightItem (qvariant_cast<TransferItem2>(right.data()));

    return leftItem.getTag() < rightItem.getTag();
}

