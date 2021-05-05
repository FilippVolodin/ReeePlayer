#ifndef APP_H
#define APP_H

#include <QSettings>

class Project;
class Library;

struct SubsCollection
{
    std::vector<QString> files;
    std::vector<int> indices;
};

class App
{
public:
    App();
    ~App();

    void open_dir(const QString&);

    QString get_recent_dir() const;

    Library* get_library();

    QSettings* get_settings();
    QVariant get_setting(const QString&, const QString&) const;
    void set_setting(const QString&, const QString&, const QVariant&);

    libvlc_instance_t* get_vlc_instance() const;

    SubsCollection get_subtitles(const QString& video_file);
    void save_subtitle_priority(const QString& video_file, const SubsCollection&);
private:
    SubsCollection get_subtitles(const QString& video_file, const QString& priorities);

    libvlc_instance_t* m_vlc_inst = nullptr;

    std::unique_ptr<QSettings> m_settings;
    std::unique_ptr<Library> m_library;
};

#endif // !APP_H

