#include "pch.h"
#include "file_manager.h"

namespace fs
{

    FileManager::FileManager()
    {
    }


    FileManager::~FileManager()
    {
    }

    void scan_folder(const QString& path, QSet<QString>& files)
    {
        QDir dir(path);
        dir.setFilter(
            QDir::NoDotAndDotDot | QDir::Dirs | QDir::Files | QDir::NoSymLinks);

        QFileInfoList list = dir.entryInfoList();
        for (const QFileInfo& file_info : list)
        {
            if (file_info.isDir())
            {
                scan_folder(file_info.canonicalFilePath(), files);
            }
            else
            {
                QString suffix = file_info.suffix().toLower();
                if (suffix == "mp4" || suffix == "mkv")
                {
                    files.insert(file_info.canonicalFilePath());
                }
            }
        }
    }

    QSet<QString> get_video_files(const QString& base_path)
    {
        QSet<QString> files;
        scan_folder(base_path, files);
        return files;
    }

    QString get_subtitles(const QString& video_file,
        const QString& lang, bool accept_wo_postfix)
    {
        // TODO support different subtitles formats

        QFileInfo info(video_file);
        QString base_file = info.absolutePath() + "/" + info.completeBaseName();

        QString srt_file = QString("%1.%2.srt").arg(base_file).arg(lang);
        if (QFileInfo(srt_file).exists())
            return srt_file;

        if (!accept_wo_postfix)
            return QString();

        srt_file = QString("%1.srt").arg(base_file);
        if (QFileInfo(srt_file).exists())
            return srt_file;
        else
            return QString();
    }

}