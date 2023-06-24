#include "syncs/gui/Twoways/SyncTableView.h"

#include "platform/Platform.h"
#include "PlatformStrings.h"
#include "MenuItemAction.h"

#include <QHeaderView>
#include <QMenu>
#include <QtConcurrent/QtConcurrent>

SyncTableView::SyncTableView(QWidget *parent)
    : QTableView(parent),
    mSyncController(this),
    mIsFirstTime(true)
{
    setIconSize(QSize(24, 24));

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &SyncTableView::customContextMenuRequested, this, &SyncTableView::onCustomContextMenuRequested);
    connect(this, &SyncTableView::pressed, this, &SyncTableView::onCellClicked);
}

void SyncTableView::keyPressEvent(QKeyEvent *event)
{
    // implement smarter row based navigation
    switch(event->key())
    {
        case Qt::Key_Left:
        case Qt::Key_Right:
            event->ignore();
            return;
        case Qt::Key_Tab:
            if(currentIndex().row() >= (model()->rowCount() - 1))
                selectRow(0);
            else
                selectRow(currentIndex().row() + 1);
            return;
        case Qt::Key_Backtab:
            if(currentIndex().row() <= 0)
                selectRow(model()->rowCount() - 1);
            else
                selectRow(currentIndex().row() - 1);
            return;
        default:
            QTableView::keyPressEvent(event);
    }
}

void SyncTableView::showEvent(QShowEvent *event)
{
    if(mIsFirstTime)
    {
        Q_UNUSED(event);
        initTable();
        mIsFirstTime = false;
    }
}

void SyncTableView::initTable()
{
    setItemDelegateForColumn(SyncItemModel::Column::MENU, new MenuItemDelegate(this));
    setItemDelegateForColumn(SyncItemModel::Column::LNAME, new IconMiddleDelegate(this));
    setItemDelegateForColumn(SyncItemModel::Column::RNAME, new ElideMiddleDelegate(this));

    horizontalHeader()->resizeSection(SyncItemModel::Column::ENABLED, FIXED_COLUMN_WIDTH);
    horizontalHeader()->resizeSection(SyncItemModel::Column::MENU, FIXED_COLUMN_WIDTH);
    horizontalHeader()->resizeSection(SyncItemModel::Column::MENU, FIXED_COLUMN_WIDTH);

    horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    horizontalHeader()->setSectionResizeMode(SyncItemModel::Column::ENABLED, QHeaderView::Fixed);
    horizontalHeader()->setSectionResizeMode(SyncItemModel::Column::MENU, QHeaderView::Fixed);
    horizontalHeader()->resizeSection(SyncItemModel::Column::LNAME, (width() - FIXED_COLUMN_WIDTH * 2) / 2);

    horizontalHeader()->setSectionResizeMode(SyncItemModel::Column::LNAME, QHeaderView::Interactive);
    horizontalHeader()->setSectionResizeMode(SyncItemModel::Column::RNAME, QHeaderView::Stretch);

    setFont(QFont().defaultFamily());

    // Hijack the sorting on the dots MENU column and hide the sort indicator,
    // instead of showing a bogus sort on that column;
    connect(horizontalHeader(), &QHeaderView::sortIndicatorChanged, this, [this](int index, Qt::SortOrder order)
    {
        if (index == SyncItemModel::Column::MENU)
            horizontalHeader()->setSortIndicator(-1, order);
    });

    // Sort by sync name by default
    sortByColumn(SyncItemModel::Column::LNAME, Qt::AscendingOrder);
    setSortingEnabled(true);
    horizontalHeader()->setSortIndicator(SyncItemModel::Column::LNAME, Qt::AscendingOrder);
}

void SyncTableView::onCustomContextMenuRequested(const QPoint &pos)
{
    QModelIndex index = indexAt(pos);
    if(index.isValid() && (index.column() > SyncItemModel::Column::ENABLED))
        showContextMenu(viewport()->mapToGlobal(pos), index);
}

void SyncTableView::onCellClicked(const QModelIndex &index)
{
    if(index.isValid() && (index.column() != SyncItemModel::Column::ENABLED))
       selectionModel()->setCurrentIndex(model()->index(index.row(), SyncItemModel::Column::ENABLED), QItemSelectionModel::NoUpdate);
    if(index.isValid() && (index.column() == SyncItemModel::Column::MENU))
        showContextMenu(QCursor().pos(), index);
}

void SyncTableView::showContextMenu(const QPoint &pos, const QModelIndex index)
{
    QMenu *menu(new QMenu(this));
    Platform::getInstance()->initMenu(menu, "SyncContextMenu");
    menu->setAttribute(Qt::WA_DeleteOnClose);

    // Show in file explorer action
    auto showLocalAction (new MenuItemAction(PlatformStrings::fileExplorer(),
                                             QIcon(QString::fromUtf8("://images/show_in_folder_ico.png"))));
    connect(showLocalAction, &MenuItemAction::triggered, this, [index]()
    {
        auto sync = index.data(Qt::UserRole).value<std::shared_ptr<SyncSettings>>();
        Utilities::openUrl(QUrl::fromLocalFile(sync->getLocalFolder()));
    });

    // Show in Mega web action
    auto showRemoteAction (new MenuItemAction(tr("Open in MEGA"),
                                              QIcon(QString::fromUtf8("://images/ico_open_MEGA.png"))));
    connect(showRemoteAction, &MenuItemAction::triggered, this, [index]()
    {
        auto sync = index.data(Qt::UserRole).value<std::shared_ptr<SyncSettings>>();
        Utilities::openInMega(sync->getMegaHandle());
    });

    // Remove Sync action
    auto delAction (new MenuItemAction(tr("Remove synced folder"),
                                       QIcon(QString::fromUtf8("://images/ico_Delete.png"))));
    delAction->setAccent(true);
    connect(delAction, &MenuItemAction::triggered, this, [this, index]()
    {
        auto sync = index.data(Qt::UserRole).value<std::shared_ptr<SyncSettings>>();
        emit removeSync(sync);
    });


    showLocalAction->setParent(menu);
    showRemoteAction->setParent(menu);
    delAction->setParent(menu);

    menu->addAction(showLocalAction);
    menu->addAction(showRemoteAction);
    menu->addSeparator();
    menu->addAction(delAction);

    menu->popup(pos);
}

MenuItemDelegate::MenuItemDelegate(QObject *parent) : QStyledItemDelegate(parent)
{
}

MenuItemDelegate::~MenuItemDelegate()
{
}

void MenuItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyleOptionViewItem opt(option);
    opt.decorationAlignment = index.data(Qt::TextAlignmentRole).value<Qt::Alignment>();
    opt.decorationPosition = QStyleOptionViewItem::Top;
    if(!opt.state.testFlag(QStyle::State_Enabled) && opt.state.testFlag(QStyle::State_Selected))
    {
        opt.state.setFlag(QStyle::State_Enabled, true);
    }
    QStyledItemDelegate::paint(painter, opt, index);
}

IconMiddleDelegate::IconMiddleDelegate(QObject* parent) :
    QStyledItemDelegate(parent)
{
}

IconMiddleDelegate::~IconMiddleDelegate()
{
}

void IconMiddleDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyledItemDelegate::paint(painter, option, index);
    QStyleOptionViewItem opt(option);
    opt.decorationAlignment = Qt::AlignVCenter | Qt::AlignHCenter;
    opt.decorationPosition = QStyleOptionViewItem::Top;
    QRect rect = option.rect;
    rect.setRight(60);
    QIcon icon = index.data(Qt::DecorationRole).value<QIcon>();

    QIcon::Mode iconMode = QIcon::Normal;
    if(option.state & QStyle::State_Selected)
    {
        iconMode = QIcon::Selected;
    }
    icon.paint(painter, rect, Qt::AlignVCenter | Qt::AlignHCenter, iconMode);
    QString text = index.data(Qt::DisplayRole).toString();
    QRect textRect = option.rect;
    textRect.setLeft(60);
    QTextOption textOption;
    textOption.setAlignment(Qt::AlignVCenter);

    QPalette::ColorGroup cg = option.state & QStyle::State_Enabled
                          ? QPalette::Normal : QPalette::Disabled;

    if (option.state & QStyle::State_Selected) {
        painter->setPen(option.palette.color(cg, QPalette::HighlightedText));
    } else {
        painter->setPen(option.palette.color(cg, QPalette::Text));
    }

    QString elidedText = option.fontMetrics.elidedText(text, Qt::ElideMiddle, textRect.width());

    painter->drawText(textRect, elidedText, textOption);

}

void IconMiddleDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
    QStyledItemDelegate::initStyleOption(option, index);
    option->icon = QIcon();
    option->text = QString();
}

ElideMiddleDelegate::ElideMiddleDelegate(QObject *parent) :
    QStyledItemDelegate(parent)
{

}

ElideMiddleDelegate::~ElideMiddleDelegate()
{

}

void ElideMiddleDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    QStyledItemDelegate::paint(painter, option, index);

    QString elidedText = option.fontMetrics.elidedText(index.data(Qt::DisplayRole).toString(), Qt::ElideMiddle, option.rect.width());

    QTextOption textAlign;
    textAlign.setAlignment(Qt::AlignVCenter);
    QRect textRect = option.rect;
    textRect.setLeft(option.rect.left() + 6);
    painter->drawText(textRect, elidedText, textAlign);
}

void ElideMiddleDelegate::initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const
{
    QStyledItemDelegate::initStyleOption(option, index);
    option->text = QString();
}
