#include "video_download_dialog.h"

// TODO directory, save settings, update directory

VideoDownloadDialog::VideoDownloadDialog(QWidget *parent)
    : QDialog(parent)
{
    ui.setupUi(this);
}

VideoDownloadDialog::~VideoDownloadDialog()
{
}

void VideoDownloadDialog::closeEvent(QCloseEvent* event)
{
    if (m_processing)
        event->ignore();
    else
        event->accept();
}

void VideoDownloadDialog::on_btnCancel_clicked()
{
    close();
}

void VideoDownloadDialog::log(const QString& str)
{
    ui.edtReport->appendPlainText(str);
    ui.edtReport->ensureCursorVisible();
}

void VideoDownloadDialog::on_btnDownload_clicked()
{
    QStringList args;
    args //<< "--skip-download"
        << "--sub-langs" << ui.edtSubtitles->text()
        << "--sub-format" << "vtt/srt"
        << "--write-subs"
        << "-S" << QString("height:%d").arg(ui.edtResolution->value())
        << "--paths" << "e:\\dev\\TestAudio";

    QString urls = ui.edtURLs->toPlainText();
    QStringList url_list = urls.split(QRegularExpression("\r\n|\r|\n"), Qt::SkipEmptyParts);
    args.append(url_list);

    ui.btnDownload->setEnabled(false);
    ui.btnCancel->setEnabled(false);
    m_processing = true;

    m_yt_dlp = std::make_unique<QProcess>();
    QObject::connect(m_yt_dlp.get(), &QProcess::started, [this]() {log("Process yt_dlp started"); });

    QObject::connect(m_yt_dlp.get(), &QProcess::errorOccurred,
        [this]()
        {
            log("Error Occurred");
            ui.btnDownload->setEnabled(true);
            ui.btnCancel->setEnabled(true);
            m_processing = false;
        }
    );

    QObject::connect(m_yt_dlp.get(), &QProcess::readyReadStandardOutput,
        [this]()
        {
            log(m_yt_dlp->readAllStandardOutput().trimmed());
        }
    );

    QObject::connect(m_yt_dlp.get(), &QProcess::readyReadStandardError,
        [this]()
        {
            log(m_yt_dlp->readAllStandardError().trimmed());
        }
    );

    QObject::connect(m_yt_dlp.get(), &QProcess::finished,
        [this]()
        {
            log("Finished");
            ui.btnDownload->setEnabled(true);
            ui.btnCancel->setEnabled(true);
            m_processing = false;
        }
    );

    m_yt_dlp->start("yt-dlp.exe", args);
}
