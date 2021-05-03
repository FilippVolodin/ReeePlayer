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

    QAction action1("Repeat selected", this);
    connect(&action1, &QAction::triggered, this, &VideoTreeView::repeat_selected);
    menu.addAction(&action1);

    menu.exec(mapToGlobal(pos));
}
