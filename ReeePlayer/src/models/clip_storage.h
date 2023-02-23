#ifndef ICLIP_H
#define ICLIP_H

#include <time_types.h>
#include <srs_interfaces.h>

class File;
class Library;

namespace srs
{
    class IFactory;
}

using ReadClipException = std::exception;

struct ClipUserData
{
    std::time_t begin = 0;
    std::time_t end = 0;
    std::vector<QString> subtitles;
    bool is_favorite = false;
};

class Clip
{
    friend File;

public:

    Clip();

    const File* get_file() const;
    File* get_file();

    QString get_uid() const;
    void set_uid(const QString&);
    void generate_uid();

    TimePoint get_adding_time() const;
    void set_adding_time(TimePoint);

    void add_repeat(TimePoint);
    const std::vector<TimePoint>& get_repeats() const;
    void set_repeats(std::vector<TimePoint>);

    const ClipUserData* get_user_data() const;
    void set_user_data(std::unique_ptr<ClipUserData>);

    srs::ICard* get_card();
    const srs::ICard* get_card() const;
    void set_card(srs::ICardUPtr);

private:
    File* m_file = nullptr;
    QString m_uid;
    TimePoint m_added;
    std::vector<TimePoint> m_repeats;
    srs::ICardUPtr m_card;
    std::unique_ptr<ClipUserData> m_user_data;

    void set_file(File*);
};

using ClipsPtr = std::shared_ptr<std::vector<Clip*>>;

struct ClipCmp
{
    bool operator()(const Clip* l, const Clip* r) const
    {
        return l->get_user_data()->begin < r->get_user_data()->begin;
    };
};

//using Clips = std::multiset<Clip*, ClipCmp>;
using Clips = std::vector<Clip*>;

struct FileUserData
{
    int player_time = 0;
    int length = 0;
};

class File
{
public:
    File(Library*, const QString& path);
    ~File();

    Library* get_library();
    const Library* get_library() const;

    QString get_path() const;

    int get_num_clips() const;
    const Clips& get_clips() const;

    void add_clip(Clip*);
    void remove_clip(Clip*);

    const FileUserData* get_user_data() const;
    void set_user_data(std::unique_ptr<FileUserData>);

private:
    QString m_path;
    Clips m_clips;
    Library* m_library = nullptr;
    std::unique_ptr<FileUserData> m_user_data;
};

class IClipStorage
{
public:
    virtual std::vector<Clip*> get_all_clips() const = 0;
    virtual std::vector<Clip*> get_all_repeatable_clips() const = 0;
    virtual std::vector<Clip*> find(QStringView, int max) const = 0;
};

bool export_txt(const std::vector<Clip*>&, const QString& filename);

File* load_file(Library* library, const QString& path, const srs::IFactory* card_factory);
void save_file(const File* file);

#endif // !ICLIP_H

