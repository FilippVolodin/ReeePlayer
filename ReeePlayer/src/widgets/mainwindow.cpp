#include "pch.h"
#include "mainwindow.h"
#include "models/clip_storage.h"
#include "models/library.h"
#include "models/app.h"
#include "models/jumpcutter.h"
#include "models/vad.h"
#include "models/session.h"
#include "library_tree_model.h"
#include "clips_view_model.h"
#include "player_window.h"
#include "waiting_dialog.h"
#include "video_download_dialog.h"
#include "about_window.h"
#include "stats_window.h"
#include "search_dialog.h"

constexpr const char* MAIN_WINDOW_GEOMETRY_KEY = "main_window_geometry";
constexpr const char* MAIN_WINDOW_STATE_KEY = "main_window_state";
constexpr const char* SPLITTER_STATE_KEY = "splitter_state";

constexpr int MAX_FOUND_CLIPS = 100;

const static QStringList ThemeNames = { "Light", "Dark" };

MainWindow::MainWindow(App* app, QWidget *parent)
    : QMainWindow(parent), m_app(app)
{
    ui.setupUi(this);

    QWidget* spacer = new QWidget();
    spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    ui.mainToolBar->addWidget(spacer);

    QLabel* lblTheme = new QLabel("Theme: ", this);
    ui.mainToolBar->addWidget(lblTheme);

    QComboBox* cbTheme = new QComboBox(this);
    for (const QString& t : ThemeNames)
        cbTheme->addItem(t);

    QString theme = app->get_setting("gui", "theme", "Light").toString();
    int theme_id = ThemeNames.indexOf(theme);
    if (theme_id == -1)
        theme_id = 0;

    cbTheme->setCurrentIndex(theme_id);
    connect(cbTheme, &QComboBox::currentIndexChanged, this, &MainWindow::on_theme_changed);
    ui.mainToolBar->addWidget(cbTheme);
    apply_theme(static_cast<Theme>(theme_id));

    QLabel* lblPlayer = new QLabel(" Player: ", this);
    ui.mainToolBar->addWidget(lblPlayer);

    QComboBox* cbPlayer = new QComboBox(this);
    cbPlayer->addItem("VLC");
    cbPlayer->addItem("Web");
    int player_engine_id = app->get_setting("main", "player", (int)PLAYER_ENGINE::Web).toInt();
    cbPlayer->setCurrentIndex(player_engine_id);
    connect(cbPlayer, &QComboBox::currentIndexChanged,
        [app](int index)
        {
            app->set_setting("main", "player", index);
        });
    ui.mainToolBar->addWidget(cbPlayer);

    connect(ui.actOpenDir, &QAction::triggered,
        this, &MainWindow::on_actOpenDir_triggered);
    connect(ui.videos, &VideoTreeView::dir_dropped,
        this, &MainWindow::open_dir);

    connect(ui.actShowAboutWindow, &QAction::triggered,
        this, &MainWindow::on_actShowAboutWindow_triggered);

    connect(ui.actReloadDir, &QAction::triggered,
        this, &MainWindow::on_actReloadDir_triggered);

    connect(ui.actExit, &QAction::triggered,
        this, &MainWindow::close);

    connect(ui.actRepeatClips, &QAction::triggered,
        this, &MainWindow::on_actRepeatClips_triggered);

    connect(ui.videos, &VideoTreeView::repeat_selected,
        this, &MainWindow::on_repeat_selected_triggered);

    connect(ui.actShowStats, &QAction::triggered,
        this, &MainWindow::on_actShowStats_triggered);

    connect(ui.videos, &VideoTreeView::stats_on_selected,
        this, &MainWindow::on_stats_on_selected_triggered);

    connect(ui.actSearchClips, &QAction::triggered,
        this, &MainWindow::on_actSearchClips_triggered);

    connect(ui.videos, &VideoTreeView::search_in_selected,
        this, &MainWindow::on_search_in_selected_triggered);

    connect(ui.videos, &VideoTreeView::download,
        this, &MainWindow::on_videotree_download_triggered);

    connect(ui.actDownload, &QAction::triggered,
        this, &MainWindow::on_actDownload_triggered);

    connect(ui.actCreateBackup, &QAction::triggered,
        this, &MainWindow::on_actCreateBackup_triggered);

    connect(ui.videos, &VideoTreeView::selection_changed,
        this, &MainWindow::on_videos_selection_changed);

    connect(ui.videos, &QAbstractItemView::doubleClicked,
        this, &MainWindow::on_videos_doubleClicked);

    connect(ui.tblClips, &QTableView::doubleClicked,
        this, &MainWindow::on_tblClips_doubleClicked);

    m_clips_model = std::make_unique<ClipModel>(this);
    m_clips_model->set_show_path(false);
    ui.tblClips->setModel(m_clips_model.get());

    connect(ui.tblClips->horizontalHeader(), &QHeaderView::sectionResized, [this](int, int, int)
        {
            ui.tblClips->resizeRowsToContents();
        });

    ui.tblClips->setColumnWidth(0, 20);
    ui.tblClips->setColumnWidth(1, 300);
    ui.tblClips->setColumnWidth(2, 300);
    ui.tblClips->setColumnWidth(3, 80);
    ui.tblClips->setColumnWidth(4, 40);

    m_library_tree = std::make_unique<LibraryTree>(this);
    ui.videos->setModel(m_library_tree.get());

    ui.videos->setColumnWidth(0, 500);
    ui.videos->setColumnWidth(1, 50);
    ui.videos->setColumnWidth(2, 50);

    connect(ui.videos, &QTreeView::expanded,
        this, &MainWindow::on_library_view_expanded);
    connect(ui.videos, &QTreeView::collapsed,
        this, &MainWindow::on_library_view_collapsed);

    QStringList col_widths =
        m_app->get_setting("gui", "videos_columns_widths").toString().split(',');
    for (int i = 0; i < col_widths.size(); ++i)
    {
        bool ok;
        int w = col_widths[i].toInt(&ok);
        if (ok)
            ui.videos->setColumnWidth(i, w);
    }

    col_widths =
        m_app->get_setting("gui", "clips_columns_widths").toString().split(',');
    for (int i = 0; i < col_widths.size(); ++i)
    {
        bool ok;
        int w = col_widths[i].toInt(&ok);
        if (ok)
            ui.tblClips->setColumnWidth(i, w);
    }

    QByteArray g = m_app->get_setting("gui", MAIN_WINDOW_GEOMETRY_KEY).toByteArray();
    QByteArray s = m_app->get_setting("gui", MAIN_WINDOW_STATE_KEY).toByteArray();
    QByteArray ss = m_app->get_setting("gui", SPLITTER_STATE_KEY).toByteArray();
    restoreGeometry(g);
    restoreState(s);
    ui.splitter->restoreState(ss);

    set_state(State::ProjectNotLoaded);

    open_dir(m_app->get_recent_dir());
}

MainWindow::~MainWindow()
{
    QStringList col_widths;
    int cols = ui.videos->model()->columnCount();
    col_widths.reserve(cols);
    for (int i = 0; i < cols; ++i)
        col_widths.push_back(QString::number(ui.videos->columnWidth(i)));
    m_app->set_setting("gui", "videos_columns_widths", col_widths.join(','));

    QStringList clips_col_widths;
    int clip_cols = ui.tblClips->model()->columnCount();
    clips_col_widths.reserve(clip_cols);
    for (int i = 0; i < clip_cols; ++i)
        clips_col_widths.push_back(QString::number(ui.tblClips->columnWidth(i)));
    m_app->set_setting("gui", "clips_columns_widths", clips_col_widths.join(','));

    m_app->set_setting("gui", MAIN_WINDOW_GEOMETRY_KEY, saveGeometry());
    m_app->set_setting("gui", MAIN_WINDOW_STATE_KEY, saveState());
    m_app->set_setting("gui", SPLITTER_STATE_KEY, ui.splitter->saveState());
}

void MainWindow::closeEvent(QCloseEvent*)
{
    qApp->quit();
}

void MainWindow::open_dir(const QString& dir)
{

    if (!dir.isEmpty())
    {
        m_clips_model->clear();

        m_app->open_dir(dir);
        Library* library = m_app->get_library();
        if (library)
        {
            m_library_tree->set_library(library);

            ui.videos->expand_folder();
            set_state(State::ProjectLoaded);
        }
    }
}

void MainWindow::on_actOpenDir_triggered()
{
    QString dir = QFileDialog::getExistingDirectory(this,
        tr("Select directory"),
        QString(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
    
    if (!dir.isEmpty())
        open_dir(dir);
}

void MainWindow::on_actReloadDir_triggered()
{
    open_dir(m_app->get_library()->get_root_path());
}

void MainWindow::on_actShowAboutWindow_triggered()
{
    AboutWindow().exec();
}

void MainWindow::on_actRepeatClips_triggered()
{
    LibraryItem* root = m_library_tree->get_item(ui.videos->rootIndex());
    repeat(get_files({ root }));
}

void MainWindow::on_repeat_selected_triggered()
{
    repeat(get_selected_files());
}

void MainWindow::on_videotree_download_triggered()
{
    QModelIndexList list = ui.videos->selectionModel()->selectedIndexes();
    if (list.size() == 1)
    {
        const LibraryItem* item = m_library_tree->get_item(list.first());
        if (item->get_item_type() == ItemType::Folder)
        {
            download_to(item->get_dir_path());
        }
    }
}

void MainWindow::on_actShowStats_triggered()
{
    const LibraryItem* root = m_library_tree->get_item(ui.videos->rootIndex());
    std::vector<File*> files = get_files({ root });

    StatsWindow* w = new StatsWindow(files, this);
    w->show();
}

void MainWindow::on_stats_on_selected_triggered()
{
    StatsWindow* w = new StatsWindow(get_selected_files(), this);
    w->show();
}

void MainWindow::on_actSearchClips_triggered()
{
    std::vector<LibraryItem*> items = { m_app->get_library()->get_root() };
    SearchDialog* w = new SearchDialog(m_app, items,  this);
    w->show();
}

void MainWindow::on_search_in_selected_triggered()
{
    SearchDialog* w = new SearchDialog(m_app, get_selected_items(),  this);
    w->show();
}

void MainWindow::on_actDownload_triggered()
{
    download_to(m_app->get_library()->get_root_path());
}

void MainWindow::on_actCreateBackup_triggered()
{
    QString root = m_app->get_library()->get_root_path();
    QDir root_dir(root);

    QDateTime cur = QDateTime::currentDateTime();
    QString backup_file = QString("%1_%2.%3").
        arg(root_dir.dirName()).
        arg(cur.toString("yyyy_MM_dd_hh_mm_ss")).
        arg("zip");

    QString backup_path = root_dir.filePath(backup_file);
    QString caption = "Save Backup File As...";
    QString filter = "Zip files (*.zip);;All files (*)";
    backup_path = QFileDialog::getSaveFileName(this, caption, backup_path, filter);
    if (!backup_path.isEmpty())
    {
        QStringList args;
        args << "a" << backup_path << "-ir!*.json";

        QProcess zip;
        zip.setWorkingDirectory(root);
        zip.start("7za", args);
        bool err = false;
        if (!zip.waitForStarted())
            err = true;

        if(!err && !zip.waitForFinished(-1))
            err = true;

        err = err || (zip.exitCode() != 0);
        if (!err)
            QMessageBox::information(this, "Info", QString("Backup file created\n%1").arg(backup_path));
        else
            QMessageBox::information(this, "Info", QString("Error while creating backup file"));
    }
}

void MainWindow::on_videos_doubleClicked(const QModelIndex& index)
{
    LibraryItem* item = m_library_tree->get_item(index);
    if (item->get_item_type() == ItemType::File)
    {
        watch(item->get_file());
    }
}

void MainWindow::on_videos_selection_changed()
{
    update_clips();
}

void MainWindow::on_tblClips_doubleClicked(const QModelIndex& index)
{
    Clip* clip = m_clips_model->get_clip(index.row());
    hide();

    std::shared_ptr<IClipQueue> queue =
        std::make_shared<WatchClipQueue>(m_app->get_library(), clip);

    getPlayerWindow()->run(Mode::WatchingClip, queue);
    update_clips();
}

void MainWindow::on_player_window_destroyed()
{
    m_player_window = nullptr;

    show();
    LibraryItem* root = m_library_tree->get_item(ui.videos->rootIndex());
    if (root)
    {
        root->update_clips_count();
    }
}

void MainWindow::on_library_view_expanded(const QModelIndex& index)
{
    m_library_tree->expanded(index);
}

void MainWindow::on_library_view_collapsed(const QModelIndex& index)
{
    m_library_tree->collapsed(index);
}

void MainWindow::on_theme_changed(int index)
{
    m_app->set_setting("gui", "theme", ThemeNames[index]);
    apply_theme(static_cast<Theme>(index));
}

void MainWindow::apply_theme(Theme theme)
{
    switch (theme)
    {
    case Theme::Dark:
    {
        QPalette p;
        p.setColor(QPalette::Window, QColor(53, 53, 53));
        p.setColor(QPalette::WindowText, Qt::white);
        p.setColor(QPalette::Disabled, QPalette::WindowText, QColor(127, 127, 127));
        p.setColor(QPalette::Base, QColor(42, 42, 42));
        p.setColor(QPalette::Disabled, QPalette::Base, QColor(53, 53, 53));
        p.setColor(QPalette::AlternateBase, QColor(66, 66, 66));
        p.setColor(QPalette::ToolTipBase, Qt::white);
        p.setColor(QPalette::ToolTipText, Qt::white);
        p.setColor(QPalette::Text, Qt::white);
        p.setColor(QPalette::Disabled, QPalette::Text, QColor(127, 127, 127));
        p.setColor(QPalette::Dark, QColor(35, 35, 35));
        p.setColor(QPalette::Shadow, QColor(20, 20, 20));
        p.setColor(QPalette::Button, QColor(53, 53, 53));
        p.setColor(QPalette::ButtonText, Qt::white);
        p.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(127, 127, 127));
        p.setColor(QPalette::BrightText, Qt::red);
        p.setColor(QPalette::Link, QColor(42, 130, 218));
        p.setColor(QPalette::Highlight, QColor(42, 130, 218));
        p.setColor(QPalette::Disabled, QPalette::Highlight, QColor(80, 80, 80));
        p.setColor(QPalette::HighlightedText, Qt::white);
        p.setColor(QPalette::Disabled, QPalette::HighlightedText, QColor(127, 127, 127));
        QApplication::setPalette(p);
        break;
    }
    default:
    {
        QApplication::setPalette(QPalette());
    }
    }
}

void MainWindow::set_state(State state)
{
    m_state = state;
    set_default_ui(state);
}

void MainWindow::set_default_ui(State state)
{
    switch (state)
    {
    case State::ProjectNotLoaded:
        ui.actReloadDir->setEnabled(false);
        ui.actRepeatClips->setEnabled(false);
        ui.actDownload->setEnabled(false);
        break;
    case State::ProjectLoaded:
        ui.actReloadDir->setEnabled(true);
        ui.actRepeatClips->setEnabled(true);
        ui.actDownload->setEnabled(true);
        break;
    }
}

PlayerWindow* MainWindow::getPlayerWindow()
{
    if (!m_player_window)
    {
        m_player_window = new PlayerWindow(m_app);
        m_player_window->setAttribute(Qt::WA_DeleteOnClose);
        Qt::WindowFlags f = m_player_window->windowFlags();
        m_player_window->setWindowFlags(f | Qt::Dialog);
        m_player_window->setWindowModality(Qt::ApplicationModal);
        connect(m_player_window, &QObject::destroyed,
            this, &MainWindow::on_player_window_destroyed);
    }
    return m_player_window;
}

QSize MainWindow::load_size(const QString& key) const
{
    QStringList window_size = m_app->get_setting("gui", key).toStringList();
    if (window_size.size() == 2)
    {
        int w = window_size[0].toInt();
        int h = window_size[1].toInt();
        if (w != 0 && h != 0)
            return QSize(w, h);
    }

    return QSize();
}

void MainWindow::save_size(const QString& key, QSize s) const
{
    m_app->set_setting("gui", key, QString("%1,%2").arg(s.width()).arg(s.height()));
}

void MainWindow::watch(File* file)
{
    //QString media_file = file->get_path();


    //bool vol_exists = is_vol_exist(media_file);
    //std::shared_ptr<VAD> vad = std::make_shared<VAD>(get_vad_file(media_file));

    //if (!vol_exists || !vad->is_ready())
    //{
    //    QFutureWatcher<void> future_watcher;
    //    WaitingDialog wd;
    //    std::function<void(QString)> log = [&](QString info)
    //    {
    //        QMetaObject::invokeMethod(&wd, "append_info", Q_ARG(QString, info));
    //    };

    //    QObject::connect(&future_watcher, &QFutureWatcher<void>::finished, &wd, &QDialog::accept);
    //    QString temp_wav;
    //    future_watcher.setFuture(QtConcurrent::run([&]()
    //    {
    //        temp_wav = create_wav(media_file, log);
    //        if (!vol_exists && !temp_wav.isEmpty())
    //        {
    //            QFileInfo fi(file->get_path());
    //            QString vol_filename = fi.absolutePath() + "/" + fi.completeBaseName() + ".vol";
    //            create_vol_file(temp_wav, vol_filename, log);
    //        }
    //    }));

    //    wd.exec();

    //    future_watcher.waitForFinished();
    //    
    //    if (!vad->run(temp_wav))
    //        vad.reset();
    //}

    hide();

    PlayerWindow* pw = getPlayerWindow();
    //pw->set_vad(vad);

    std::shared_ptr<IClipQueue> queue =
        std::make_shared<AddingClipsQueue>(m_app->get_library(), file, m_app->get_srs_factory());
    pw->run(Mode::Watching, queue);
    update_clips();
}

void MainWindow::repeat(std::vector<File*> files)
{
    std::shared_ptr<IClipQueue> queue =
        std::make_shared<RepeatingClipQueueV2>(m_app->get_library(), files);
    
    if (queue->overdue_count() != 0)
    {
        hide();
        getPlayerWindow()->run(Mode::Repeating, queue);
        update_clips();
    }
    else
    {
        QMessageBox::information(this, tr("Information"),
            tr("No clips to repeat"));
    }
}

void MainWindow::download_to(const QString& dir)
{
    VideoDownloadDialog vdd;

    vdd.set_dir(dir);
    vdd.set_subtitles(m_app->get_setting("download", "subtitles", "en.*, ru.*").toString());
    vdd.set_resolution(m_app->get_setting("download", "resolution", 720).toInt());

    vdd.exec();
    if (vdd.accepted())
    {
        m_app->set_setting("download", "subtitles", vdd.get_subtitles());
        m_app->set_setting("download", "resolution", vdd.get_resolution());
        open_dir(m_app->get_library()->get_root_path());
    }
}

void MainWindow::update_clips()
{
    ClipsPtr clips = std::make_shared<std::vector<Clip*>>();

    QModelIndexList list = ui.videos->selectionModel()->selectedIndexes();
    if (list.size() == 1)
    {
        const QModelIndex& index = list.first();
        LibraryItem* item = m_library_tree->get_item(index);
        if (item->get_item_type() == ItemType::File)
        {
            auto cs = item->get_file()->get_clips();
            std::copy_if(cs.begin(), cs.end(), std::back_inserter(*clips),
                [](Clip* clip) {return !clip->is_removed(); });
        }
    }
    m_clips_model->set_clips(clips);
    ui.tblClips->resizeRowsToContents();
}

std::vector<LibraryItem*> MainWindow::get_selected_items() const
{
    QModelIndexList list = ui.videos->selectionModel()->selectedIndexes();
    std::vector<LibraryItem*> items;
    items.reserve(list.size());
    for (const QModelIndex& index : list)
    {
        LibraryItem* item = m_library_tree->get_item(index);
        items.push_back(item);
    }
    return get_disjoint_items(items);
}

std::vector<File*> MainWindow::get_selected_files() const
{
    QModelIndexList list = ui.videos->selectionModel()->selectedIndexes();
    std::vector<const LibraryItem*> items;
    items.reserve(list.size());
    for (const QModelIndex& index : list)
    {
        const LibraryItem* item = m_library_tree->get_item(index);
        items.push_back(item);
    }

    return get_files(items);
}
