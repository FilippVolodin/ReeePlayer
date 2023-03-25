#include "video_download_dialog.h"

VideoDownloadDialog::VideoDownloadDialog(QWidget *parent)
    : QDialog(parent)
{
    ui.setupUi(this);
    ui.btnStop->setEnabled(false);
    this->setWindowTitle(tr("Download video"));
}

VideoDownloadDialog::~VideoDownloadDialog()
{
}

QString VideoDownloadDialog::get_dir() const
{
    QDir dir(ui.edtDir->text());
    return dir.absolutePath();
}

void VideoDownloadDialog::set_dir(const QString& dir)
{
    ui.edtDir->setText(dir);
}

QString VideoDownloadDialog::get_subtitles() const
{
    return ui.edtSubtitles->text();
}

void VideoDownloadDialog::set_subtitles(const QString& subs)
{
    ui.edtSubtitles->setText(subs);
}

int VideoDownloadDialog::get_resolution() const
{
    return ui.edtResolution->value();
}

void VideoDownloadDialog::set_resolution(int res)
{
    return ui.edtResolution->setValue(res);
}

bool VideoDownloadDialog::accepted() const
{
    return m_accepted;
}

void VideoDownloadDialog::closeEvent(QCloseEvent* event)
{
    if (m_processing)
        event->ignore();
    else
        event->accept();
}

void VideoDownloadDialog::on_btnStop_clicked()
{
    if (m_yt_dlp && m_yt_dlp->state() == QProcess::Running)
    {
        QString cmd = QString("TASKKILL /f /t /PID %1").arg(m_yt_dlp->processId());
        WinExec(cmd.toLocal8Bit().data(), SW_HIDE);
    }
}

void VideoDownloadDialog::on_btnClose_clicked()
{
    close();
}

void VideoDownloadDialog::on_btnSelectDir_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(this,
        tr("Select directory"),
        ui.edtDir->text(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (!dir.isEmpty())
        ui.edtDir->setText(dir);
}

void VideoDownloadDialog::log(const QString& str)
{
    ui.edtReport->appendPlainText(str);
    ui.edtReport->ensureCursorVisible();
}

void VideoDownloadDialog::on_btnDownload_clicked()
{
    m_accepted = true;

    if (ui.edtURLs->toPlainText().trimmed().isEmpty())
    {
        log(tr("Enter one or more URLs"));
        return;
    }

    QDir dir(ui.edtDir->text());
    if (!dir.exists())
    {
        bool res = dir.mkpath(".");
        if (!res)
        {
            log(QString(tr("Can't create directory: %1").arg(ui.edtDir->text())));
            return;
        }
    }

    QStringList args;
    args //<< "--skip-download"
        << "--sub-langs" << ui.edtSubtitles->text()
        << "--sub-format" << "vtt/srt"
        << "--write-subs"
        << "-S" << QString("height:%1").arg(ui.edtResolution->value())
        << "--paths" << ui.edtDir->text();

    QString urls = ui.edtURLs->toPlainText();
    QStringList url_list = urls.split(QRegularExpression("\r\n|\r|\n"), Qt::SkipEmptyParts);
    args.append(url_list);

    ui.btnDownload->setEnabled(false);
    ui.btnStop->setEnabled(true);
    ui.btnClose->setEnabled(false);
    m_processing = true;

    m_yt_dlp = std::make_unique<QProcess>();
    QObject::connect(m_yt_dlp.get(), &QProcess::started, [this]() {log("Process yt_dlp started"); });

    QObject::connect(m_yt_dlp.get(), &QProcess::errorOccurred,
        [this]()
        {
            log("Error Occurred");
            ui.btnDownload->setEnabled(true);
            ui.btnStop->setEnabled(false);
            ui.btnClose->setEnabled(true);
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
            ui.btnStop->setEnabled(false);
            ui.btnClose->setEnabled(true);
            m_processing = false;
        }
    );

    m_yt_dlp->start("utils/yt-dlp.exe", args);
}
