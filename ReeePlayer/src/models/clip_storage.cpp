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
    m_uid = uid;
}

void Clip::generate_uid()
{
    set_uid(QUuid::createUuid().toString(QUuid::WithoutBraces));
}

TimePoint Clip::get_adding_time() const
{
    return m_added;
}

void Clip::set_adding_time(TimePoint time)
{
    m_added = time;
}

void Clip::add_repeat(TimePoint time)
{
    m_repeats.push_back(time);
}

const std::vector<TimePoint>& Clip::get_repeats() const
{
    return m_repeats;
}

void Clip::set_repeats(std::vector<TimePoint> repeats)
{
    m_repeats = std::move(repeats);
}

//QString Clip::get_subtitle(int index) const
//{
//    if (index >= 0 && index < m_subtitles.size())
//        return m_subtitles[index];
//    else
//        return QString();
//}
//
//const std::vector<QString>& Clip::get_subtitles() const
//{
//    return m_subtitles;
//}
//
//void Clip::set_subtitles(std::vector<QString>&& subtitles)
//{
//    if (subtitles != m_subtitles)
//    {
//        m_subtitles = std::move(subtitles);
//        if (m_file)
//            m_file->get_library()->clip_changed(this);
//    }
//}
//
//bool Clip::is_favorite() const
//{
//    return m_is_favorite;
//}
//
//void Clip::set_favorite(bool favorite)
//{
//    if (favorite != m_is_favorite)
//    {
//        m_is_favorite = favorite;
//        if (m_file)
//            m_file->get_library()->clip_changed(this);
//    }
//}

const ClipUserData* Clip::get_user_data() const
{
    return m_user_data.get();
}

void Clip::set_user_data(std::unique_ptr<ClipUserData> data)
{
    m_user_data = std::move(data);
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
    m_clips.push_back(clip);
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
    delete clip;
}

const FileUserData* File::get_user_data() const
{
    return m_user_data.get();
}

void File::set_user_data(std::unique_ptr<FileUserData> user_data)
{
    m_user_data = std::move(user_data);
}

bool export_txt(const std::vector<Clip*>& clips, const QString& filename)
{
    QFile file(filename);
    QTextStream out(&file);
    if (!file.open(QIODevice::WriteOnly))
        return false;

    for (const Clip* clip : clips)
    {
        const ClipUserData* data = clip->get_user_data();
        std::vector<QString> subs = data->subtitles;

        out << clip->get_uid() << "\r\n";
        out << subs[0].replace("\r\n", "<br>").replace("\n", "<br>") << "\r\n";
        out << subs[1].replace("\r\n", "<br>").replace("\n", "<br>") << "\r\n";
        out << "\r\n";
    }
    return true;
}

srs::ICardUPtr read_card(const QJsonObject& json_clip, int version, const srs::ICardFactory* card_factory)
{
    bool res = true;
    srs::ICardUPtr card;
    if (version > 1)
    {
        if (json_clip.contains("card") && json_clip["card"].isObject())
        {
            QJsonObject json_card = json_clip["card"].toObject();
            if (json_card.contains("type") && json_card["type"].isString())
            {
                QString type = json_card["type"].toString();
                card = card_factory->create(type);
                card->read(json_card);
            }
            else
                res = false;
        }
        else
            res = false;
    }
    else
    {
        // Legacy
        card = card_factory->create("simple");
        card->read(json_clip);
    }

    if (res)
        return card;
    else
        return nullptr;
}

std::unique_ptr<FileUserData> read_file_user_data(const QJsonObject& json_user_data)
{
    std::unique_ptr<FileUserData> data = std::make_unique<FileUserData>();
    if (json_user_data.contains("player_time") && json_user_data["player_time"].isDouble())
        data->player_time = json_user_data["player_time"].toInt();
    if (json_user_data.contains("length") && json_user_data["length"].isDouble())
        data->length = json_user_data["length"].toInt();
    return data;
}

std::unique_ptr<FileUserData> read_file_user_data(const QJsonObject& json, int version)
{
    if (version > 1 && json.contains("user_data") && json["user_data"].isObject())
    {
        QJsonObject json_user_data = json["user_data"].toObject();
        return read_file_user_data(json_user_data);
    }
    else
        return read_file_user_data(json);
}

std::unique_ptr<ClipUserData> read_clip_user_data(const QJsonObject& json_user_data)
{
    std::unique_ptr<ClipUserData> data = std::make_unique<ClipUserData>();
    data->subtitles.resize(2);
    if (json_user_data.contains("begin") && json_user_data["begin"].isDouble())
        data->begin = json_user_data["begin"].toInt();
    if (json_user_data.contains("end") && json_user_data["end"].isDouble())
        data->end = json_user_data["end"].toInt();
    if (json_user_data.contains("favorite") && json_user_data["favorite"].isBool())
        data->is_favorite = json_user_data["favorite"].toBool();
    if (json_user_data.contains("text1") && json_user_data["text1"].isString())
        data->subtitles[0] = json_user_data["text1"].toString();
    if (json_user_data.contains("text2") && json_user_data["text2"].isString())
        data->subtitles[1] = json_user_data["text2"].toString();
    return data;
}

std::unique_ptr<ClipUserData> read_clip_user_data(const QJsonObject& json_clip, int version)
{
    if (version > 1 && json_clip.contains("user_data") && json_clip["user_data"].isObject())
    {
        QJsonObject json_user_data = json_clip["user_data"].toObject();
        return read_clip_user_data(json_user_data);
    }
    else
        return read_clip_user_data(json_clip);
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

    int version = 1;
    if (json.contains("version") && json["version"].isDouble())
        version = json["version"].toInt();

    file->set_user_data(read_file_user_data(json, version));

    if (json.contains("clips") && json["clips"].isArray())
    {
        QJsonArray json_clips = json["clips"].toArray();
        for (int i = 0; i < json_clips.size(); ++i)
        {
            try
            {
                int res = true;
                QJsonObject json_clip = json_clips[i].toObject();

                // TODO possible leak
                Clip* clip = new Clip();

                srs::ICardUPtr card = read_card(json_clip, version, card_factory);
                clip->set_card(std::move(card));

                if (json_clip.contains("uid") && json_clip["uid"].isString())
                    clip->set_uid(json_clip["uid"].toString());
                if (json_clip.contains("added") && json_clip["added"].isDouble())
                    clip->set_adding_time(TimePoint(Duration(json_clip["added"].toInteger())));
                if (json_clip.contains("repeats") && json_clip["repeats"].isArray())
                {
                    QJsonArray repeats_arr = json_clip["repeats"].toArray();
                    std::vector<TimePoint> repeats;
                    repeats.reserve(repeats_arr.size());
                    for (const QJsonValue& value : repeats_arr)
                    {
                        if (value.isDouble())
                            repeats.push_back(TimePoint(Duration(value.toInteger())));
                    }
                    clip->set_repeats(std::move(repeats));
                }
                
                clip->set_user_data(read_clip_user_data(json_clip, version));

                file->add_clip(clip);
            }
            catch (srs::ReadException)
            {
            }
        }
    }
    return file;
}

void save_file(const File* file)
{
    QJsonObject json;
    json["version"] = 2;

    QJsonObject json_file_user_data;
    json_file_user_data["player_time"] = file->get_user_data()->player_time;
    json_file_user_data["length"] = file->get_user_data()->length;
    json["user_data"] = json_file_user_data;

    QJsonArray json_clips;
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
        if (!repeats_arr.empty())
        {
            json_clip["repeats"] = repeats_arr;
        }

        QJsonObject json_card;
        if (clip->get_card() != nullptr)
            clip->get_card()->write(json_card);
        json_clip["card"] = json_card;

        const ClipUserData* data = clip->get_user_data();
        QJsonObject json_clip_user_data;
        json_clip_user_data["begin"] = data->begin;
        json_clip_user_data["end"] = data->end;
        if (data->is_favorite)
            json_clip_user_data["favorite"] = data->is_favorite;
        json_clip_user_data["text1"] = data->subtitles[0];
        json_clip_user_data["text2"] = data->subtitles[1];
        json_clip["user_data"] = json_clip_user_data;

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