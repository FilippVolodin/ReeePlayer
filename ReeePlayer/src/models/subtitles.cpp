#include "pch.h"
#include "subtitles.h"
//#include "srtparser.h"

#include <QByteArray>

SubtitlesManager::SubtitlesManager()
{
}


SubtitlesManager::~SubtitlesManager()
{
    for (auto& subtitle : m_subtitles)
        delete subtitle;
}

void SubtitlesManager::load(const QString& filename)
{
    for (auto& subtitle : m_subtitles)
        delete subtitle;

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly))
        return;

    QTextStream in(&file);
    //in.setCodec("UTF-8");
    //in.setAutoDetectUnicode(false);

    SubtitlesParser parser;
    m_subtitles = parser.read(in);
}

const Subtitle * SubtitlesManager::get(int index) const
{
    if (index < 0 || index >= m_subtitles.size())
        return nullptr;
    return m_subtitles[index];
}

const Subtitle * SubtitlesManager::find(int time, bool check_end_time) const
{
    auto comp = [](const Subtitle* s, int time) -> bool {
        return s->time.begin < time;
    };

    auto it = std::lower_bound(m_subtitles.begin(), m_subtitles.end(), time, comp);
    if (it == m_subtitles.begin())
        it = m_subtitles.end();
    else
        --it;

    if (it != m_subtitles.end())
    {
        const Subtitle* subtitle = *it;
        if (!check_end_time || time < subtitle->time.end)
            return subtitle;
    }

    return nullptr;
}

Subtitles SubtitlesParser::read(QTextStream& in) const
{
    Subtitles subtitles;

    const Subtitle* subtitle = nullptr;
    while ((subtitle = read_subtitle(in)) != nullptr)
    {
        const_cast<Subtitle*>(subtitle)->index =
            static_cast<int>(subtitles.size());
        subtitles.push_back(subtitle);
    }
    return subtitles;
}

const Subtitle* SubtitlesParser::read_subtitle(QTextStream& in) const
{
    Subtitle* res = nullptr;

    int counter = 0;
    Subtitle::TimeRange time;
    QString text;

    ParserState state = ParserState::Unknown;
    while (!in.atEnd() && state != ParserState::Finish)
    {
        QString line = in.readLine().trimmed();

        if (state == ParserState::Unknown)
            state = get_state(line);

        switch (state)
        {
        case ParserState::WebVTTBlock:
            if (line.isEmpty())
            {
                state = ParserState::Unknown;
            }
            break;
        case ParserState::Id:
            if (!line.isEmpty())
            {
                counter = line.toInt();
                state = ParserState::Time;
            }
            break;
        case ParserState::Time:
            time = parse_time_range(line);
            state = ParserState::Text;
            break;
        case ParserState::Text:
            if (!line.isEmpty())
            {
                if (!text.isEmpty())
                    text += "<br/>";
                text += line;
            }
            else
            {
                state = ParserState::Finish;
            }
            break;
        }
    }

    if (state == ParserState::Finish)
    {
        res = new Subtitle;
        res->counter = counter;
        res->time = time;

        QTextDocument doc;
        doc.setHtml(text);
        res->text = doc.toPlainText();
    }

    return res;
}

Subtitle::TimeRange SubtitlesParser::parse_time_range(const QString& str) const
{
    Subtitle::TimeRange res;
    int sep_pos = str.indexOf("-->");
    if (sep_pos == -1)
        return res;

    QString begin_str = str.left(sep_pos).trimmed();
    QString end_str   = str.mid(sep_pos + 3).trimmed();
    
    static const QString FORMATS[] =
        { "hh:mm:ss,zzz", "hh:mm:ss.zzz", "mm:ss,zzz", "mm:ss.zzz" };
    
    QTime begin_time;
    for (const QString& format : FORMATS)
    {
        begin_time = QTime::fromString(begin_str, format);
        if (begin_time.isValid())
            break;
    }

    QTime end_time;
    for (const QString& format : FORMATS)
    {
        end_time = QTime::fromString(end_str, format);
        if (end_time.isValid())
            break;
    }

    res.begin = begin_time.msecsSinceStartOfDay();
    res.end   = end_time.msecsSinceStartOfDay();
    return res;
}

ParserState SubtitlesParser::get_state(const QString & line) const
{
    if (line == "WEBVTT" ||
        line == "NOTE" ||
        line == "STYLE" ||
        line == "REGION")
        return ParserState::WebVTTBlock;
    else
        return ParserState::Id;
}
