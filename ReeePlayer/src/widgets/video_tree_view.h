#pragma once

class VideoTreeView : public QTreeView
{
    Q_OBJECT

public:
    VideoTreeView(QWidget* parent);
    ~VideoTreeView();

    void expand_folder();

signals:
    void drag_finished();
    void dir_dropped(const QString& dir);

    void repeat_selected();
    void stats_on_selected();
    void search_in_selected();
    void download();
    void selection_changed();
protected:
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent *event) override;
    void selectionChanged(const QItemSelection& selected, const QItemSelection& deselected);
private:

    void expand_folder(const QModelIndex & index);
    void show_context_menu(const QPoint&);

    bool m_pressed_already_selected = false;
    bool m_drop_left = false;
};
