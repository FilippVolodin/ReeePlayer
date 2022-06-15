#include "clips_view.h"

ClipsView::ClipsView(QWidget* parent) : QTableView(parent)
{
    connect(this, &ClipsView::customContextMenuRequested,
        this, &ClipsView::show_context_menu);
}

ClipsView::~ClipsView()
{
}

void ClipsView::show_context_menu(const QPoint& pos)
{
    QMenu menu(tr("Context menu"), this);

    QAction export_action("Export selected", this);
    connect(&export_action, &QAction::triggered, this, &ClipsView::export_selected);
    menu.addAction(&export_action);

    QAction repeat_action("Repeat selected", this);
    repeat_action.setIcon(QIcon(":/MainWindow/repeat_clips"));
    connect(&repeat_action, &QAction::triggered, this, &ClipsView::repeat_selected);
    menu.addAction(&repeat_action);

    menu.exec(mapToGlobal(pos));
}
