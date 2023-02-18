#include "pch.h"
#include "search_dialog.h"
#include "models/app.h"
#include "models/library.h"
#include "models/session.h"
#include "clips_view_model.h"
#include "player_window.h"
#include "export_dialog.h"
#include "clips_view.h"

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

    int file_num = 0;
    for (const LibraryItem* item : m_items)
    {
        item->iterate_files([&file_num](File*) {file_num++; });
    }

    bool search_everywhere = m_items.size() == 1 && m_items[0] == m_app->get_library()->get_root();
    if (search_everywhere)
    {
        ui.lblSearchIn->setText(QString("<b>All %1 Files</b>").arg(file_num));
    }
    else
    {
        ui.lblSearchIn->setText(QString("<b>%1 Selected Files</b>").arg(file_num));
    }

    connect(ui.tblClips, &ClipsView::export_selected,
        this, &SearchDialog::on_export_selected_triggered);
    connect(ui.tblClips, &ClipsView::repeat_selected,
        this, &SearchDialog::on_repeat_selected_triggered);
}

SearchDialog::~SearchDialog()
{
}

void SearchDialog::on_btnExport_clicked()
{
    export_clips(*m_clips.get());
}

void SearchDialog::on_btnRepeat_clicked()
{
    if (!m_clips || m_clips->empty())
        return;

    std::shared_ptr<IClipSession> session =
        std::make_shared<RepeatingSession>(m_app->get_library(), *m_clips.get());
    get_player_window()->run(Mode::Repeating, session);
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

    std::shared_ptr<IClipSession> session =
        std::make_shared<WatchClipSession>(m_app->get_library(), clip);

    get_player_window()->run(Mode::WatchingClip, session);
}

void SearchDialog::on_export_selected_triggered()
{
    std::vector<Clip*> clips = get_selected_clips();
    if (clips.empty())
        return;
    export_clips(clips);
}

void SearchDialog::on_repeat_selected_triggered()
{
    std::vector<Clip*> clips = get_selected_clips();
    if (clips.empty())
        return;

    std::shared_ptr<IClipSession> session =
        std::make_shared<RepeatingSession>(m_app->get_library(), clips);
    get_player_window()->run(Mode::Repeating, session);
}

void SearchDialog::search(const QString& text)
{
    m_clips = std::make_shared<std::vector<Clip*>>();
    for (const LibraryItem* item : m_items)
        item->find_clips(text, 0, ui.chkFavorite->isChecked(), *m_clips);

    m_clips_model->set_clips(m_clips);
    ui.tblClips->resizeRowsToContents();
    ui.btnExport->setEnabled(!m_clips->empty());
    ui.btnRepeat->setEnabled(!m_clips->empty());
}

void SearchDialog::export_clips(const std::vector<Clip*>& clips)
{
    ExportDialog export_dialog;
    export_dialog.set_info_text(QString("Export %1 clips to Anki file").arg(clips.size()));
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

    QProgressDialog progress("Generating media files...", "Abort", 0, clips.size(), this);
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
        for (const Clip* clip : clips)
        {
            const ClipUserData* user_data = clip->get_user_data();
            progress.setValue(clip_idx);
            if (progress.wasCanceled())
                break;

            if (has_audio)
            {
                QStringList args1;
                args1 << "-y"
                    << "-ss" << QString::number(user_data->begin * 0.001)
                    << "-to" << QString::number(user_data->end * 0.001)
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
                    << "-ss" << QString::number((user_data->begin + user_data->end) * 0.0005)
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
    progress.setValue(clips.size());

    progress.setLabelText("Create Anki File...");

    QString temp_txt = QDir::temp().absoluteFilePath("clips.txt");
    export_txt(clips, temp_txt);

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

std::vector<Clip*> SearchDialog::get_selected_clips() const
{
    QModelIndexList list = ui.tblClips->selectionModel()->selectedRows();
    std::vector<Clip*> clips;
    QSortFilterProxyModel* proxy_model = static_cast<QSortFilterProxyModel*>(ui.tblClips->model());
    for (const QModelIndex& proxy_index : list)
    {
        QModelIndex index = proxy_model->mapToSource(proxy_index);
        clips.push_back((*m_clips)[index.row()]);
    }
    return clips;
}

PlayerWindow* SearchDialog::get_player_window()
{
    PlayerWindow* player_window = new PlayerWindow(m_app, this);
    player_window->setAttribute(Qt::WA_DeleteOnClose);
    player_window->setWindowModality(Qt::ApplicationModal);
    return player_window;
}
