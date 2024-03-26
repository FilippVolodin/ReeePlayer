#pragma once

#include <qsubtitles.h>
#include <app.h>
#include <rewinder.h>

class App;

class SubtitlesList : public Rewinder
{
public:
    SubtitlesList(App* app, const QString& media_file);
    const qsubs::ISubtitles* fetch(int id, const QString& subs_file);
    const qsubs::ISubtitles* get_primary() const;
    const std::vector<QString>& get_files() const;
    const std::vector<QString>& get_short_names() const;
    int get_preferred_file_index(int id) const;

    std::optional<PlaybackTime> rewind(PlaybackTime, PlaybackTimeDiff) const override;

private:
    void id_assign(int index, const qsubs::ISubtitles*);

    SubsCollection m_subs;
    std::vector<QString> m_subtitle_files;
    std::vector<QString> m_short_sub_names;
    std::map<QString, qsubs::ISubtitlesPtr> m_subtitles;
    std::vector<const qsubs::ISubtitles*> m_subtitles_index;
};