#ifndef ICLIP_H
#define ICLIP_H

#include <time_types.h>
#include <srs_icard.h>

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

    auto operator<=>(const ClipUserData&) const = default;
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

    bool is_removed() const;
    TimePoint get_removal_time() const;
    void set_removal_time(TimePoint);
    void restore();

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
    std::unique_ptr<ClipUserData> m_user_data;
    std::optional<TimePoint> m_removed;
    srs::ICardUPtr m_card;

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

template<typename T>
auto uptr_const_indirect(const std::unique_ptr<T>& ptr) -> const T*
{
    return ptr.get();
}

template<typename T>
auto uptr_indirect(const std::unique_ptr<T>& ptr) -> T*
{
    return ptr.get();
}

class File
{
public:

    File(Library*, const QString& path);
    ~File() = default;

    Library* get_library();
    const Library* get_library() const;

    QString get_path() const;

    int get_num_clips(bool count_removed = false) const;
    auto get_clips() const
    {
        return m_clips
            | std::ranges::views::transform(uptr_const_indirect<Clip>);
    }
    auto get_clips()
    {
        return m_clips
            | std::ranges::views::transform(uptr_indirect<Clip>);
    }

    void add_clip(std::unique_ptr<Clip>);
    void remove_clip(Clip*);

    const FileUserData* get_user_data() const;
    void set_user_data(std::unique_ptr<FileUserData>);

private:
    QString m_path;
    std::vector<std::unique_ptr<Clip>> m_clips;
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

std::unique_ptr<File> load_file(Library* library, const QString& path, const srs::IFactory* card_factory);
void save_file(const File* file);

#endif // !ICLIP_H

