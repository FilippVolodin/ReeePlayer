#include "pch.h"
#include "search_dialog.h"
#include "models/app.h"
#include "models/library.h"
#include "clips_view_model.h"
#include "player_window.h"
#include "export_dialog.h"

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

    bool search_everywhere = m_items.size() == 1 && m_items[0] == m_app->get_library()->get_root();
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

void SearchDialog::on_btnExport_clicked()
{
    if (!m_clips)
        return;

    ExportDialog export_dialog;
    export_dialog.set_info_text(QString("Export %1 clips to Anki file").arg(m_clips->size()));
    export_dialog.exec();
    if (export_dialog.result() != QDialog::Accepted)
        return;

    QString deck_name = export_dialog.get_deck_name();
    bool has_screenshot = export_dialog.is_screenshot_included();
    bool has_audio = export_dialog.is_audio_included();

    QString root = m_app->get_library()->get_root_path();
    QString default_file = QDir(root).absoluteFilePath(QString("%1.apkg").arg(deck_name));
    QString anki_file = QFileDialog::getSaveFileName(this,
        "Export to Anki",
        default_file,
        "Anki deck files (*.apkg)");

    if (anki_file.isEmpty())
        return;

    QProgressDialog progress("Generating media files...", "Abort", 0, m_clips->size(), this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(1000);

    QString temp_dir;
    if (has_screenshot || has_audio)
    {
        QDir dir = QDir::temp();
        dir.mkdir("ReeePlayer");
        dir.cd("ReeePlayer");
        temp_dir = dir.absolutePath();

        int clip_idx = 0;
        for (const Clip* clip : *m_clips)
        {
            progress.setValue(clip_idx);
            if (progress.wasCanceled())
                break;

            if (has_audio)
            {
                QStringList args1;
                args1 << "-y"
                    << "-ss" << QString::number(clip->get_begin() * 0.001)
                    << "-to" << QString::number(clip->get_end() * 0.001)
                    << "-i" << clip->get_file()->get_path()
                    << dir.absoluteFilePath(QString("%1.mp3").arg(clip->get_uid()));

                QProcess ffmpeg_process1;
                ffmpeg_process1.start("ffmpeg/ffmpeg.exe", args1);
                ffmpeg_process1.waitForStarted();
                ffmpeg_process1.waitForFinished();
            }

            if (has_screenshot)
            {
                QStringList args2;
                args2 << "-y"
                    << "-ss" << QString::number((clip->get_begin() + clip->get_end()) * 0.0005)
                    << "-i" << clip->get_file()->get_path()
                    << "-vframes" << "1"
                    << dir.absoluteFilePath(QString("%1.jpg").arg(clip->get_uid()));

                QProcess ffmpeg_process2;
                ffmpeg_process2.start("ffmpeg/ffmpeg.exe", args2);
                ffmpeg_process2.waitForStarted();
                ffmpeg_process2.waitForFinished();
            }
            clip_idx++;
        }
    }
    progress.setValue(m_clips->size());

    progress.setLabelText("Create Anki File...");

    QString temp_txt = QDir::temp().absoluteFilePath("clips.txt");
    export_txt(*m_clips, temp_txt);

    QFileInfo fi(anki_file);

    QStringList args;
    args << temp_txt << temp_dir;

    if (has_audio)
        args << "-vc";

    if (has_screenshot)
        args << "-tb";

    args << anki_file << deck_name;

    QProcess anki_process;
    anki_process.start("pyutils/anki.exe", args);
    if (anki_process.waitForStarted())
    {
        if (anki_process.waitForFinished(-1))
        {
            if (anki_process.exitCode() == 0)
                QMessageBox::information(this, "Info", "Export completed succesfully");
            else
                QMessageBox::critical(this, "Info", "Export completed with error");
        }
        else
        {
            QMessageBox::critical(this, "Error", "Can't finish pyutils/anki.exe");
        }
    }
    else
    {
        QMessageBox::critical(this, "Error", "Can't start pyutils/anki.exe");
    }
}

void SearchDialog::on_edtText_returnPressed()
{
    search(ui.edtText->text());
}

void SearchDialog::on_btnSearch_clicked()
{
    search(ui.edtText->text());
}

void SearchDialog::on_tblClips_doubleClicked(const QModelIndex& proxy_index)
{
    QSortFilterProxyModel* proxy_model = static_cast<QSortFilterProxyModel*>(ui.tblClips->model());
    QModelIndex index = proxy_model->mapToSource(proxy_index);

    Clip* clip = m_clips_model->get_clip(index.row());
    PlayerWindow* player_window = new PlayerWindow(m_app, this);
    player_window->setAttribute(Qt::WA_DeleteOnClose);
    player_window->setWindowModality(Qt::ApplicationModal);
    player_window->watch_clip(clip);
}

void SearchDialog::search(const QString& text)
{
    m_clips = std::make_shared<std::vector<Clip*>>();
    for (const LibraryItem* item : m_items)
        item->find_clips(text, 0, ui.chkFavorite->isChecked(), *m_clips);

    m_clips_model->set_clips(m_clips);
    ui.tblClips->resizeRowsToContents();
}