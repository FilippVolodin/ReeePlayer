#ifndef CLIPS_VIEW_H
#define CLIPS_VIEW_H

class ClipsView : public QTableView
{
    Q_OBJECT

public:
    ClipsView(QWidget* parent);
    ~ClipsView();
signals:
    void export_selected();
    void repeat_selected();
private:
    void show_context_menu(const QPoint&);
};

#endif