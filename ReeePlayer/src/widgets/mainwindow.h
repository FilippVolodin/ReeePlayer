#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "ui_mainwindow.h"

class App;
class PlayerWindow;
class LibraryTree;
class ClipModel;
class File;
class Clip;

class MainWindow : public QMainWindow
{
	Q_OBJECT

public:
    MainWindow(App*, QWidget *parent = nullptr);
    ~MainWindow();
protected:
    void closeEvent(QCloseEvent* event);
private:

    enum class State { ProjectNotLoaded, ProjectLoaded };

    void open_dir(const QString&);

    void on_actOpenDir_triggered();
    void on_actReloadDir_triggered();

    void on_actRepeatClips_triggered();
    void on_repeat_selected_triggered();
    void on_videotree_download_triggered();

    void on_actDownload_triggered();

    void on_videos_itemClicked(QTreeWidgetItem *item, int column);
    void on_videos_doubleClicked(const QModelIndex &index);
    void on_tblClips_doubleClicked(const QModelIndex &index);

    void on_btnFindClips_clicked();

    void on_player_window_destroyed();

    void on_library_view_expanded(const QModelIndex& index);
    void on_library_view_collapsed(const QModelIndex& index);

    void set_state(State);
    void set_default_ui(State);

    PlayerWindow* getPlayerWindow();

    QSize load_size(const QString&) const;
    void save_size(const QString&, QSize) const;

    void watch(File* file);
    void repeat(std::vector<File*>);
    void download_to(const QString& dir);

    Ui::MainWindowClass ui;
    
    State m_state = State::ProjectNotLoaded;

    PlayerWindow* m_player_window = nullptr;

    std::unique_ptr<LibraryTree> m_library_tree;
    std::unique_ptr<ClipModel> m_clips_model;
    App* m_app = nullptr;
};

#endif