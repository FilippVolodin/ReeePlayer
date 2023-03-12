#include "pch.h"

#include "library_tree_pmodel.h"
#include "models/library.h"
#include "models/library_item.h"

namespace pmodel
{

    LibraryTree::LibraryTree(QObject * parent) : QAbstractItemModel(parent)
    {
    }

    LibraryTree::~LibraryTree()
    {
    }

    int LibraryTree::columnCount(const QModelIndex&) const
    {
        return 2;
    }

    //Qt::DropActions LibraryTree::supportedDragActions() const
    //{
    //    return Qt::MoveAction | Qt::CopyAction;
    //}

    //Qt::DropActions LibraryTree::supportedDropActions() const
    //{
    //    return Qt::MoveAction | Qt::CopyAction;
    //}

    //bool LibraryTree::insertRows(int row, int count, const QModelIndex & parent)
    //{
    //    return QAbstractItemModel::insertRows(row, count, parent);
    //}

    //bool LibraryTree::removeRows(int row, int count, const QModelIndex & parent)
    //{
    //    return remove_rows(row, count, parent, false);
    //}

    void LibraryTree::set_library(Library* library)
    {
        beginResetModel();
        m_library = library;
        endResetModel();
    }

    LibraryItem* LibraryTree::get_item(const QModelIndex& index) const
    {
        if (!index.isValid())
            return m_library->get_root();
        else
            return static_cast<LibraryItem*>(index.internalPointer());
    }

    void LibraryTree::expanded(const QModelIndex& index)
    {
        LibraryItem* item = get_item(index);
        item->expand(true);
        m_library->save();
    }

    void LibraryTree::collapsed(const QModelIndex& index)
    {
        LibraryItem* item = get_item(index);
        item->expand(false);
        m_library->save();
    }

    bool LibraryTree::is_expanded(const QModelIndex& index)
    {
        LibraryItem* item = get_item(index);
        return item->is_expanded();
    }

    QVariant LibraryTree::data(const QModelIndex& index, int role) const
    {
        if (!index.isValid())
            return QVariant();

        const LibraryItem *item =
            static_cast<LibraryItem*>(index.internalPointer());

        return item->data(index.column(), role);
    }

    Qt::ItemFlags LibraryTree::flags(const QModelIndex &index) const
    {
        if (!index.isValid())
            return Qt::NoItemFlags;

        LibraryItem* item = static_cast<LibraryItem*>(index.internalPointer());
        return item->flags(index.column());
    }

    QVariant LibraryTree::headerData(int section, Qt::Orientation orientation,
        int role) const
    {
        if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
            switch (section) {
            case 0:
                return tr("Name");
            case 1:
                return tr("Clips");
            default:
                break;
            }
        }
        return QVariant();
    }

    QModelIndex LibraryTree::index(int row, int column, const QModelIndex &parent) const
    {
        if (!hasIndex(row, column, parent))
            return QModelIndex();

        LibraryItem *parentItem = get_item(parent);

        LibraryItem *childItem = parentItem->child(row);
        if (childItem)
            return createIndex(row, column, childItem);
        return QModelIndex();
    }

    QModelIndex LibraryTree::parent(const QModelIndex &child) const
    {
        if (!child.isValid())
            return QModelIndex();

        LibraryItem *childItem = static_cast<LibraryItem*>(child.internalPointer());
        LibraryItem *parentItem = childItem->parent();

        if (!parentItem || parentItem == m_library->get_root())
            return QModelIndex();

        return createIndex(parentItem->row(), 0, parentItem);
    }

    int LibraryTree::rowCount(const QModelIndex &parent) const
    {
        if (!m_library)
            return 0;

        if (!m_library->get_root())
            return 0;

        if (parent.column() > 0)
            return 0;

        LibraryItem* parent_item = get_item(parent);
        return parent_item->num_children();
    }
}