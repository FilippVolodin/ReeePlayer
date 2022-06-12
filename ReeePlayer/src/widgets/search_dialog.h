#ifndef SEARCH_DIALOG_H
#define SEARCH_DIALOG_H

#include <QWidget>
#include "ui_search_dialog.h"
#include "models/clip_storage.h"

class App;
class ClipModel;
class LibraryItem;
class PlayerWindow;

class SearchDialog : public QDialog
{
    Q_OBJECT

public:
    SearchDialog(App* app, std::vector<const LibraryItem*> items, QWidget *parent = Q_NULLPTR);
    ~SearchDialog();
public slots:
    void on_btnSearch_clicked();
    void on_btnExport_clicked();
    void on_edtText_returnPressed();
private:
    void on_tblClips_doubleClicked(const QModelIndex& index);
    void search(const QString& text);

    Ui::SearchDialog ui;

    App* m_app;
    std::vector<const LibraryItem*> m_items;
    ClipModel* m_clips_model;
    ClipsPtr m_clips;
};

#endif