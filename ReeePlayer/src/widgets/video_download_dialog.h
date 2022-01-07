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
protected:

    void closeEvent(QCloseEvent* event) override;

private slots:
    void on_btnDownload_clicked();
    void on_btnCancel_clicked();

private:
    void log(const QString&);

    Ui::VideoDownloadDialog ui;

    std::unique_ptr<QProcess> m_yt_dlp;
    bool m_processing = false;
};

#endif