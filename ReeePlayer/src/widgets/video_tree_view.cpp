#include "pch.h"
#include "video_tree_view.h"
#include "library_tree_model.h"
#include "models/clip_storage.h"

VideoTreeView::VideoTreeView(QWidget* parent)
    : QTreeView(parent)
{
    connect(this, &VideoTreeView::customContextMenuRequested,
        this, &VideoTreeView::show_context_menu);
}

VideoTreeView::~VideoTreeView()
{
}

void VideoTreeView::expand_folder()
{
    auto m = model();
    if (dynamic_cast<LibraryTree*>(m))
    {
        blockSignals(true);
        expand_folder(rootIndex());
        blockSignals(false);
    }
}

void VideoTreeView::dragMoveEvent(QDragMoveEvent * event)
{
    setDropIndicatorShown(true);
    QTreeView::dragMoveEvent(event);
}

void VideoTreeView::dropEvent(QDropEvent* event)
{
    QList<QUrl> urls = event->mimeData()->urls();
    if (!urls.isEmpty())
    {
        QUrl url = urls.first();
        if (url.isValid() && url.isLocalFile())
        {
            QString path = url.toLocalFile();
            QFileInfo info(path);
            if (info.isDir())
                emit dir_dropped(path);
        }
    }
}

void VideoTreeView::selectionChanged(const QItemSelection& selected, const QItemSelection& deselected)
{
    emit selection_changed();
    QTreeView::selectionChanged(selected, deselected);
}

void VideoTreeView::expand_folder(const QModelIndex & index)
{
    LibraryTree* m = static_cast<LibraryTree*>(model());
    if (index.isValid())
    {
        if (m->is_expanded(index))
            this->expand(index);
    }
    if (!m->hasChildren(index) || (index.flags() & Qt::ItemNeverHasChildren))
        return;
    auto rows = m->rowCount(index);
    auto cols = m->columnCount(index);
    for (int i = 0; i < rows; ++i)
        for (int j = 0; j < cols; ++j)
            expand_folder(m->index(i, j, index));
}

void VideoTreeView::show_context_menu(const QPoint& pos)
{
    QMenu menu(tr("Context menu"), this);

    QAction repeat_action("Repeat selected", this);
    repeat_action.setIcon(QIcon(":/MainWindow/repeat_clips"));
    connect(&repeat_action, &QAction::triggered, this, &VideoTreeView::repeat_selected);
    menu.addAction(&repeat_action);

    QAction stats_action("Stats on selected", this);
    stats_action.setIcon(QIcon(":/MainWindow/stats"));
    connect(&stats_action, &QAction::triggered, this, &VideoTreeView::stats_on_selected);
    menu.addAction(&stats_action);

    QAction search_action("Search in selected", this);
    search_action.setIcon(QIcon(":/MainWindow/search"));
    connect(&search_action, &QAction::triggered, this, &VideoTreeView::search_in_selected);
    menu.addAction(&search_action);

    QAction* download_action = nullptr;
    QModelIndexList list = selectionModel()->selectedIndexes();
    if (list.size() == 1)
    {
        const QModelIndex& index = list.first();
        LibraryTree* tree = static_cast<LibraryTree*>(model());
        const LibraryItem* item = tree->get_item(index);
        if (item->get_item_type() == ItemType::Folder)
        {
            download_action = new QAction(QString("Download to \"%1\"...").arg(item->get_name()), this);
            download_action->setIcon(QIcon(":/MainWindow/download"));
            connect(download_action, &QAction::triggered, this, &VideoTreeView::download);
        }

    }

    if (download_action != nullptr)
    {
        menu.addSeparator();
        menu.addAction(download_action);
    }

    menu.exec(mapToGlobal(pos));

}
