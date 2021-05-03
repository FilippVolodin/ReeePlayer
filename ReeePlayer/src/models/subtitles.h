#ifndef SUBTITLES_H
#define SUBTITLES_H

enum class ParserState { Unknown, WebVTTBlock, Id, Time, Text, Finish };

struct Subtitle
{
    //friend Subtitles;
//public:
    struct TimeRange
    {
        int begin = 0;
        int end = 0;
    };
//private:
    int index;
    int counter;
    TimeRange time;
    QString text;
};

using Subtitles = std::vector<const Subtitle*>;

class SubtitlesManager
{
public:
    SubtitlesManager();
    ~SubtitlesManager();

    void load(const QString&);
    const Subtitle* get(int) const;
    const Subtitle* find(int, bool check_end_time) const;
private:
    Subtitles m_subtitles;
};

class SubtitlesParser
{
public:
    Subtitles read(QTextStream&) const;
private:
    const Subtitle* read_subtitle(QTextStream&) const;
    Subtitle::TimeRange parse_time_range(const QString&) const;
    ParserState get_state(const QString& line) const;
};

#endif // !SUBTITLES_H

