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
protected:
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent *event) override;
private:

    void expand_folder(const QModelIndex & index);
    void show_context_menu(const QPoint&);

    bool m_pressed_already_selected = false;
    bool m_drop_left = false;
};
