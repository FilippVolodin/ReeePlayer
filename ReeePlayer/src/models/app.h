#ifndef APP_H
#define APP_H

#include <QSettings>
#include <srs.h>

class Project;
class Library;

class IVideoWidget;

namespace srs
{
    class IFactory;
}

struct SubsCollection
{
    std::vector<QString> files;
    std::vector<int> indices;
};

enum class PLAYER_ENGINE { VLC = 0, Web = 1 };

class App
{
public:
    App();
    ~App();

    void open_dir(const QString&);

    QString get_recent_dir() const;

    Library* get_library();

    QSettings* get_settings();
    QVariant get_setting(const QString&, const QString&, const QVariant& default_value = QVariant()) const;
    void set_setting(const QString&, const QString&, const QVariant&);

    libvlc_instance_t* get_vlc_instance() const;

    SubsCollection get_subtitles(const QString& video_file);
    void save_subtitle_priority(const QString& video_file, const SubsCollection&);

    const srs::IFactory* get_srs_factory() const;

    IVideoWidget* get_player_widget(PLAYER_ENGINE);
private:
    SubsCollection get_subtitles(const QString& video_file, const QString& priorities);

    libvlc_instance_t* m_vlc_inst = nullptr;

    std::unique_ptr<QSettings> m_settings;
    std::unique_ptr<Library> m_library;
    std::unique_ptr<srs::IFactory> m_card_factory;

    // TODO
    std::unique_ptr<IVideoWidget> m_vlc_widget;
    std::unique_ptr<IVideoWidget> m_web_widget;
};

#endif // !APP_H

