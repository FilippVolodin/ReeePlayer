#include "pch.h"
#include "clip_storage.h"
#include "library.h"
#include <srs_simple.h>
#include <srs_fsrs.h>
#include <srs.h>

void Clip::set_file(File* file)
{
    m_file = file;
}

Clip::Clip()
{
}

const File* Clip::get_file() const
{
    return m_file;
}

File* Clip::get_file()
{
    return m_file;
}

QString Clip::get_uid() const
{
    return m_uid;
}

void Clip::set_uid(const QString& uid)
{
    if (uid != m_uid)
    {
        m_uid = uid;
        if (m_file)
            m_file->get_library()->clip_changed(this);
    }
}

void Clip::generate_uid()
{
    set_uid(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

std::time_t Clip::get_begin() const
{
    return m_begin;
}

void Clip::set_begin(std::time_t begin)
{
    if (begin != m_begin)
    {
        m_begin = begin;
        if (m_file)
            m_file->get_library()->clip_changed(this);
    }
}

std::time_t Clip::get_end() const
{
    return m_end;
}

void Clip::set_end(std::time_t end)
{
    if (end != m_end)
    {
        m_end = end;
        if (m_file)
            m_file->get_library()->clip_changed(this);
    }
}

TimePoint Clip::get_adding_time() const
{
    return m_added;
}

void Clip::set_adding_time(TimePoint time)
{
    if (time != m_added)
    {
        m_added = time;
        if (m_file)
            m_file->get_library()->clip_changed(this);
    }
}

void Clip::add_repeat(TimePoint time)
{
    m_repeats.push_back(time);
    if (m_file)
        m_file->get_library()->clip_changed(this);
}

const std::vector<TimePoint>& Clip::get_repeats() const
{
    return m_repeats;
}

void Clip::set_repeats(std::vector<TimePoint> repeats)
{
    m_repeats = std::move(repeats);
}

QString Clip::get_subtitle(int index) const
{
    if (index >= 0 && index < m_subtitles.size())
        return m_subtitles[index];
    else
        return QString();
}

const std::vector<QString>& Clip::get_subtitles() const
{
    return m_subtitles;
}

void Clip::set_subtitles(std::vector<QString>&& subtitles)
{
    if (subtitles != m_subtitles)
    {
        m_subtitles = std::move(subtitles);
        if (m_file)
            m_file->get_library()->clip_changed(this);
    }
}

bool Clip::is_favorite() const
{
    return m_is_favorite;
}

void Clip::set_favorite(bool favorite)
{
    if (favorite != m_is_favorite)
    {
        m_is_favorite = favorite;
        if (m_file)
            m_file->get_library()->clip_changed(this);
    }
}

srs::ICard* Clip::get_card()
{
    return m_card.get();
}

const srs::ICard* Clip::get_card() const
{
    return m_card.get();
}

void Clip::set_card(srs::ICardUPtr card)
{
    m_card = std::move(card);
}

File::File(Library* library, const QString& path)
    : m_library(library), m_path(path)
{
}

File::~File()
{
    qDeleteAll(m_clips);
}

Library* File::get_library()
{
    return m_library;
}

const Library* File::get_library() const
{
    return m_library;
}

QString File::get_path() const
{
    return m_path;
}

int File::get_num_clips() const
{
    return m_clips.size();
}

const Clips& File::get_clips() const
{
    return m_clips;
}

void File::add_clip(Clip* clip)
{
    clip->set_file(this);
    m_clips.insert(clip);
    m_library->clip_added(clip);
}

void File::remove_clip(Clip* clip)
{
    for (auto it = m_clips.begin(); it != m_clips.end(); ++it)
    {
        if (*it == clip)
        {
            m_clips.erase(it);
            break;
        }
    }

    m_library->clip_removed(clip);
    delete clip;
}

int File::get_player_time() const
{
    return m_player_time;
}

void File::set_player_time(int player_time)
{
    if (player_time != m_player_time)
    {
        m_player_time = player_time;
        get_library()->file_changed(this);
    }
}

int File::get_length() const
{
    return m_length;
}

void File::set_length(int length)
{
    if (length != m_length)
    {
        m_length = length;
        get_library()->file_changed(this);
    }
}

bool export_txt(const std::vector<Clip*>& clips, const QString& filename)
{
    QFile file(filename);
    QTextStream out(&file);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    for (const Clip* clip : clips)
    {
        out << clip->get_uid() << "\r\n";
        out << clip->get_subtitle(0).replace("\r\n", "<br>").replace("\n", "<br>") << "\r\n";
        out << clip->get_subtitle(1).replace("\r\n", "<br>").replace("\n", "<br>") << "\r\n";
        out << "\r\n";
    }
    return true;
}

File* load_file(Library* library, const QString& path, const srs::ICardFactory* card_factory)
{
    File* file = new File(library, path);
    QFileInfo info(path);
    QString json_file = info.absolutePath() + "/" + info.completeBaseName() + ".user.json";

    QFile in_file(json_file);
    if (!in_file.open(QIODevice::ReadOnly))
        return file;

    QByteArray text = in_file.readAll();
    QJsonDocument doc(QJsonDocument::fromJson(text));

    QJsonObject json = doc.object();

    if (json.contains("player_time") && json["player_time"].isDouble())
        file->set_player_time(json["player_time"].toInt());

    if (json.contains("length") && json["length"].isDouble())
        file->set_length(json["length"].toInt());

    if (json.contains("clips") && json["clips"].isArray())
    {
        QJsonArray json_clips = json["clips"].toArray();
        for (int i = 0; i < json_clips.size(); ++i)
        {
            try
            {
                Clip* clip = new Clip();
                srs::ICardUPtr card;
                QString text1;
                QString text2;
                QJsonObject json_clip = json_clips[i].toObject();
                if (json_clip.contains("card") && json_clip["card"].isObject())
                {
                    QJsonObject json_card = json_clip["card"].toObject();
                    if (json_card.contains("type") && json_card["type"].isString())
                    {
                        QString type = json_card["type"].toString();
                        card = card_factory->create(type);
                        card->read(json_card);
                    }
                }
                else
                {
                    // Legacy
                    card = card_factory->create("simple");
                    card->read(json_clip);
                }
                clip->set_card(std::move(card));

                if (json_clip.contains("uid") && json_clip["uid"].isString())
                    clip->set_uid(json_clip["uid"].toString());
                if (json_clip.contains("added") && json_clip["added"].isDouble())
                    clip->set_adding_time(TimePoint(Duration(json_clip["added"].toInteger())));
                if (json_clip.contains("begin") && json_clip["begin"].isDouble())
                    clip->set_begin(json_clip["begin"].toInt());
                if (json_clip.contains("end") && json_clip["end"].isDouble())
                    clip->set_end(json_clip["end"].toInt());
                if (json_clip.contains("favorite") && json_clip["favorite"].isBool())
                    clip->set_favorite(json_clip["favorite"].toBool());
                if (json_clip.contains("repeats") && json_clip["repeats"].isArray())
                {
                    QJsonArray repeats_arr = json_clip["repeats"].toArray();
                    std::vector<TimePoint> repeats;
                    repeats.reserve(repeats_arr.size());
                    for (QJsonValue value : repeats_arr)
                    {
                        if (value.isDouble())
                            repeats.push_back(TimePoint(Duration(value.toInteger())));
                    }
                    clip->set_repeats(std::move(repeats));
                }
                if (json_clip.contains("text1") && json_clip["text1"].isString())
                    text1 = json_clip["text1"].toString();
                if (json_clip.contains("text2") && json_clip["text2"].isString())
                    text2 = json_clip["text2"].toString();
                clip->set_subtitles({ text1, text2 });
                file->add_clip(clip);
            }
            catch (srs::ReadException&)
            {
            }
        }
    }
    return file;
}

void save_file(const File* file)
{
    QJsonObject json;
    QJsonArray json_clips;

    json["player_time"] = file->get_player_time();
    json["length"] = file->get_length();

    for (const Clip* clip : file->get_clips())
    {
        const std::vector<TimePoint>& repeats = clip->get_repeats();
        QJsonArray repeats_arr;
        std::transform(repeats.begin(), repeats.end(), std::back_inserter(repeats_arr),
            [](const TimePoint& tp) {return tp.time_since_epoch().count(); });

        QJsonObject json_clip;
        json_clip["uid"] = clip->get_uid();
        if (clip->get_adding_time() != TimePoint(Duration::zero()))
            json_clip["added"] = clip->get_adding_time().time_since_epoch().count();
        json_clip["begin"] = clip->get_begin();
        json_clip["end"] = clip->get_end();
        if (clip->is_favorite())
            json_clip["favorite"] = clip->is_favorite();
        if (!repeats_arr.empty())
        {
            json_clip["repeats"] = repeats_arr;
        }
        json_clip["text1"] = clip->get_subtitle(0);
        json_clip["text2"] = clip->get_subtitle(1);

        QJsonObject json_card;
        if (clip->get_card() != nullptr)
            clip->get_card()->write(json_card);
        json_clip["card"] = json_card;

        json_clips.append(json_clip);
    }

    json["clips"] = json_clips;

    QFileInfo info(file->get_path());
    QString json_file = info.absolutePath() + "/" + info.completeBaseName() + ".user.json";

    QFile out_file(json_file);
    if (!out_file.open(QIODevice::WriteOnly))
        return;
    out_file.write(QJsonDocument(json).toJson());
}