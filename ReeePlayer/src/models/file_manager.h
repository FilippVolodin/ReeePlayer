#ifndef FILE_MANAGER
#define FILE_MANAGER

namespace fs
{

    class FileManager
    {
    public:
        FileManager();
        ~FileManager();
    };

    QSet<QString> get_video_files(const QString& base_path);
    QString get_subtitles(const QString& video_file,
        const QString& lang, bool accept_wo_postfix);
}

#endif // !FILE_MANAGER
