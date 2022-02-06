#ifndef VIDEO_DOWNLOAD_DIALOG_H
#define VIDEO_DOWNLOAD_DIALOG_H

#include <QWidget>
#include "ui_video_download_dialog.h"

class VideoDownloadDialog : public QDialog
{
    Q_OBJECT

public:
    VideoDownloadDialog(QWidget *parent = Q_NULLPTR);
    ~VideoDownloadDialog();

    QString get_dir() const;
    void set_dir(const QString&);

    QString get_subtitles() const;
    void set_subtitles(const QString&);

    int get_resolution() const;
    void set_resolution(int);

    bool accepted() const;
protected:

    void closeEvent(QCloseEvent* event) override;

private slots:
    void on_btnDownload_clicked();
    void on_btnStop_clicked();
    void on_btnClose_clicked();
    void on_btnSelectDir_clicked();

private:
    void log(const QString&);

    Ui::VideoDownloadDialog ui;

    std::unique_ptr<QProcess> m_yt_dlp;
    bool m_processing = false;
    bool m_accepted = false;
};

#endif