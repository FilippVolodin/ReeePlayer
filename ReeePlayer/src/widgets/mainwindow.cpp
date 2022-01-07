#include "pch.h"
#include "mainwindow.h"
#include "models/clip_storage.h"
#include "models/library.h"
#include "models/app.h"
#include "models/jumpcutter.h"
#include "library_tree_model.h"
#include "clips_view_model.h"
#include "player_window.h"
#include "waiting_dialog.h"
#include "video_download_dialog.h"

constexpr const char* MAIN_WINDOW_GEOMETRY_KEY = "main_window_geometry";
constexpr const char* MAIN_WINDOW_STATE_KEY = "main_window_state";
constexpr const char* PLAYER_WINDOW_GEOMETRY_KEY = "player_window_geometry";
constexpr const char* SPLITTER_STATE_KEY = "splitter_state";

constexpr int MAX_FOUND_CLIPS = 100;

MainWindow::MainWindow(App* app, QWidget *parent)
    : QMainWindow(parent), m_app(app)
{
    ui.setupUi(this);

    connect(ui.actOpenDir, &QAction::triggered,
        this, &MainWindow::on_actOpenDir_triggered);
    connect(ui.videos, &VideoTreeView::dir_dropped,
        this, &MainWindow::open_dir);

    connect(ui.actReloadDir, &QAction::triggered,
        this, &MainWindow::on_actReloadDir_triggered);

    connect(ui.actExit, &QAction::triggered,
        this, &MainWindow::close);

    connect(ui.actRepeatClips, &QAction::triggered,
        this, &MainWindow::on_actRepeatClips_triggered);

    connect(ui.videos, &VideoTreeView::repeat_selected,
        this, &MainWindow::on_repeat_selected_triggered);

    connect(ui.videos, &QAbstractItemView::doubleClicked,
        this, &MainWindow::on_videos_doubleClicked);

    connect(ui.tblClips, &QTableView::doubleClicked,
        this, &MainWindow::on_tblClips_doubleClicked);

    connect(ui.btnFindClips, &QPushButton::clicked,
        this, &MainWindow::on_btnFindClips_clicked);

    m_library_tree = std::make_unique<LibraryTree>(this);
    ui.videos->setModel(m_library_tree.get());

    m_clips_model = std::make_unique<ClipModel>(this);
    ui.tblClips->setModel(m_clips_model.get());

    connect(ui.videos, &QTreeView::expanded,
        this, &MainWindow::on_library_view_expanded);
    connect(ui.videos, &QTreeView::collapsed,
        this, &MainWindow::on_library_view_collapsed);

    ui.videos->setColumnWidth(0, 300);
    ui.videos->setColumnWidth(1, 300);

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

void MainWindow::closeEvent(QCloseEvent* event)
{
    qApp->quit();
}

void MainWindow::open_dir(const QString& dir)
{
    if (!dir.isEmpty())
    {
        m_app->open_dir(dir);
        Library* library = m_app->get_library();
        if (library != nullptr)
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

    open_dir(dir);
}

void MainWindow::on_actReloadDir_triggered()
{
    open_dir(m_app->get_library()->get_root_path());
}

void MainWindow::on_actRepeatClips_triggered()
{
    LibraryItem* root =
        m_library_tree->get_item(ui.videos->rootIndex());
    
    std::vector<const LibraryItem*> items = { root };
    repeat(get_files(items));
}

void MainWindow::on_repeat_selected_triggered()
{
    QModelIndexList list = ui.videos->selectionModel()->selectedIndexes();
    std::vector<const LibraryItem*> items;
    items.reserve(list.size());
    for (const QModelIndex& index : list)
    {
        const LibraryItem* item = m_library_tree->get_item(index);
        items.push_back(item);
    }

    repeat(get_files(items));
}

void MainWindow::on_videos_itemClicked(QTreeWidgetItem* item, int /*column*/)
{
    if (item->isSelected())
        qDebug("edit");
}

void MainWindow::on_videos_doubleClicked(const QModelIndex& index)
{
    LibraryItem* item = m_library_tree->get_item(index);
    if (item->get_item_type() == ItemType::File)
    {
        watch(item->get_file());
    }
}

void MainWindow::on_tblClips_doubleClicked(const QModelIndex& index)
{
    Clip* clip = m_clips_model->get_clip(index.row());
    hide();
    getPlayerWindow()->watch_clip(clip);
}

void MainWindow::on_btnFindClips_clicked()
{
    Library* library = m_app->get_library();
    if (library == nullptr)
        return;

    QString text = ui.edtSearchedText->text();
    std::vector<Clip*> clips = library->find_clips(text, MAX_FOUND_CLIPS);
    m_clips_model->set_clips(std::move(clips));
}

void MainWindow::on_player_window_destroyed()
{
    m_app->set_setting("gui", PLAYER_WINDOW_GEOMETRY_KEY,
        m_player_window->saveGeometry());

    m_player_window = nullptr;

    show();
    LibraryItem* root = m_library_tree->get_item(ui.videos->rootIndex());
    if (root != nullptr)
    {
        root->update_clips_count_up();
    }
}

void MainWindow::on_library_view_expanded(const QModelIndex& index)
{
    LibraryItem* item = m_library_tree->get_item(index);
    m_library_tree->expanded(index);
}

void MainWindow::on_library_view_collapsed(const QModelIndex& index)
{
    LibraryItem* item = m_library_tree->get_item(index);
    m_library_tree->collapsed(index);
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
        break;
    case State::ProjectLoaded:
        ui.actReloadDir->setEnabled(true);
        ui.actRepeatClips->setEnabled(true);
        ui.edtSearchedText->clear();
        break;
    }
}

PlayerWindow* MainWindow::getPlayerWindow()
{
    if (m_player_window == nullptr)
    {
        m_player_window = new PlayerWindow(m_app);
        m_player_window->setAttribute(Qt::WA_DeleteOnClose);
        Qt::WindowFlags f = m_player_window->windowFlags();
        m_player_window->setWindowFlags(f | Qt::Dialog);
        connect(m_player_window, &QObject::destroyed,
            this, &MainWindow::on_player_window_destroyed);

        QByteArray g = m_app->get_setting("gui", PLAYER_WINDOW_GEOMETRY_KEY).toByteArray();
        m_player_window->restoreGeometry(g);
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
    if (!is_vol_exist(file->get_path()))
    {
        QFutureWatcher<void> future_watcher;
        WaitingDialog wd;
        std::function<void(QString)> log = [&](QString info)
        {
            QMetaObject::invokeMethod(&wd, "append_info", Q_ARG(QString, info));
        };

        QObject::connect(&future_watcher, &QFutureWatcher<void>::finished, &wd, &QDialog::accept);

        future_watcher.setFuture(QtConcurrent::run([filename = file->get_path(), log]()
        {
            create_vol_file(filename, log);
        }));

        wd.exec();

        future_watcher.waitForFinished();
    }

    hide();
    getPlayerWindow()->watch(file);
}

void MainWindow::repeat(std::vector<File*> files)
{
    bool has_clips = false;
    for (const File* file : files)
    {
        if (file->get_num_clips() != 0)
        {
            has_clips = true;
            break;
        }
    }

    if (has_clips)
    {
        hide();
        getPlayerWindow()->repeat(std::move(files));
    }
    else
    {
        QMessageBox::information(this, tr("Information"),
            tr("No clips to repeat"));
    }
}
