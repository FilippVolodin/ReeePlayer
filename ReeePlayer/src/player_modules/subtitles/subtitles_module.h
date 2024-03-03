#include <player_module.h>
#include <qsubtitles.h>

#include <vector>

class App;
struct PlayerContext;

namespace pm
{
    class SubtitlesView;
}

namespace qsubs
{
    class ISubtitles;
}

class SubtitlesModule : public PlayerModule
{
public:
    SubtitlesModule(App* app, const PlayerContext*);

    void setup(QMainWindow* player_window);
    void show_video(const QString& filename);
    void time_changed(int);
private:
    void set_subtitles(int index, const QString& filename);
    void update_cue(int index);

    pm::SubtitlesView* m_view = nullptr;

    App* m_app = nullptr;
    const PlayerContext* m_player_context = nullptr;
    std::vector<QString> m_subtitle_files;
    std::shared_ptr<const qsubs::ISubtitles> m_subtitles;
    const qsubs::ICue* m_current_cue;
};