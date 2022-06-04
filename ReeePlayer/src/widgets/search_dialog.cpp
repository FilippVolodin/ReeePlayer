#include "search_dialog.h"
#include "models/clip_storage.h"
#include "models/app.h"
#include "models/library.h"
#include "clips_view_model.h"
#include "player_window.h"

SearchDialog::SearchDialog(App* app, std::vector<const LibraryItem*> items, QWidget *parent) :
    m_app(app),
    m_items(std::move(items)),
    QDialog(parent)
{
    ui.setupUi(this);
    connect(ui.tblClips->horizontalHeader(), &QHeaderView::sectionResized, [this](int, int, int)
        {
            ui.tblClips->resizeRowsToContents();
        });

    m_clips_model = new ClipModel(this);
    ui.tblClips->setModel(m_clips_model);
    auto* hh = ui.tblClips->horizontalHeader();
    //hh->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    //hh->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    //hh->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    //hh->setSectionResizeMode(3, QHeaderView::ResizeToContents);
    //hh->setSectionResizeMode(4, QHeaderView::ResizeToContents);

    QSortFilterProxyModel* proxy_model = new QSortFilterProxyModel(this);
    proxy_model->setSourceModel(m_clips_model);
    ui.tblClips->setModel(proxy_model);

    ui.tblClips->setColumnWidth(0, 20);
    ui.tblClips->setColumnWidth(1, 300);
    ui.tblClips->setColumnWidth(2, 300);
    ui.tblClips->setColumnWidth(3, 80);
    ui.tblClips->setColumnWidth(4, 40);
    ui.tblClips->setColumnWidth(5, 500);

    connect(ui.tblClips, &QTableView::doubleClicked,
        this, &SearchDialog::on_tblClips_doubleClicked);

    bool search_everywhere = m_items.size() == 1 && m_items[0]->parent() == m_app->get_library()->get_root();
    if (search_everywhere)
    {
        ui.lblSearchIn->setText("<b>All Files</b>");
    }
    else
    {
        ui.lblSearchIn->setText("<b>Selected Files</b>");
    }
}

SearchDialog::~SearchDialog()
{
}

void SearchDialog::on_edtText_returnPressed()
{
    search(ui.edtText->text());
}

void SearchDialog::on_btnSearch_clicked()
{
    search(ui.edtText->text());
}

void SearchDialog::on_tblClips_doubleClicked(const QModelIndex& index)
{
    Clip* clip = m_clips_model->get_clip(index.row());
    PlayerWindow* player_window = new PlayerWindow(m_app, this);
    player_window->setAttribute(Qt::WA_DeleteOnClose);
    player_window->setWindowModality(Qt::ApplicationModal);
    player_window->watch_clip(clip);
}

void SearchDialog::search(const QString& text)
{
    std::vector<Clip*> clips;
    for (const LibraryItem* item : m_items)
        item->find_clips(text, 0, ui.chkFavorite->isChecked(), clips);

    m_clips_model->set_clips(std::move(clips));
    ui.tblClips->resizeRowsToContents();
}