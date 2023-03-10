#ifndef PROJECT_H
#define PROJECT_H

#include <time_types.h>

class Clip;
class File;
struct ClipUserData;
struct FileUserData;

class Library;

namespace srs
{
    class IFactory;
}

class TodayClipStat
{
public:
    TodayClipStat(const Library*, const File*);

    void inc_added();
    void inc_repeated();

    int get_added_count() const;
    int get_added_count_for_file() const;
    int get_repeated_count() const;
private:
    int m_added_count = 0;
    int m_added_count_for_file = 0;
    int m_repeated_count = 0;
};

struct ClipPriorityCmp
{
    ClipPriorityCmp(TimePoint cur_time);
    TimePoint m_cur_time;
    bool operator()(const Clip* lhs, const Clip* rhs);
};

class IClipQueue
{
public:
    virtual ~IClipQueue();

    virtual const ClipUserData* get_clip_user_data() const = 0;
    virtual void set_clip_user_data(std::unique_ptr<ClipUserData>) = 0;
    virtual void remove() = 0;

    virtual const QString get_file_path() const = 0;
    virtual const FileUserData* get_file_user_data() const = 0;
    virtual void set_file_user_data(std::unique_ptr<FileUserData>) = 0;

    virtual bool is_reviewing() const = 0;
    virtual bool has_next() const = 0;
    virtual bool has_prev() const = 0;
    virtual bool next() = 0;
    virtual bool prev() = 0;
    virtual int overdue_count() const = 0;

    virtual void repeat(int rating) = 0;

    virtual void save_library() = 0;

    virtual TodayClipStat* get_today_clip_stat() = 0;
};

class BaseClipQueue : public IClipQueue
{
public:
    BaseClipQueue(Library*);

    void remove() override;

    const ClipUserData* get_clip_user_data() const override;
    void set_clip_user_data(std::unique_ptr<ClipUserData>) override;

    const QString get_file_path() const override;
    const FileUserData* get_file_user_data() const override;
    void set_file_user_data(std::unique_ptr<FileUserData>) override;

    bool is_reviewing() const override;
    bool has_next() const override;
    bool has_prev() const override;
    bool next() override;
    bool prev() override;
    int overdue_count() const override;

    void repeat(int rating) override;

    void save_library() override;

    const TodayClipStat* get_today_clip_stat() const;
protected:
    BaseClipQueue(Library*, const File*);

    Clip* get_current_clip();
    const Clip* get_current_clip() const;
    void set_current_clip(Clip*);

    virtual const File* get_current_file() const;
    virtual File* get_current_file();

    TodayClipStat* get_today_clip_stat();
private:
    Library* m_library;
    Clip* m_current_clip = nullptr;
    std::unique_ptr<TodayClipStat> m_today_clip_stat = nullptr;
};

class RepeatingClipQueue : public BaseClipQueue
{
public:
    RepeatingClipQueue(Library*, const std::vector<File*>&);
    RepeatingClipQueue(Library*, const std::vector<Clip*>&);
    ~RepeatingClipQueue();

    void remove() override;

    bool has_next() const override;
    bool has_prev()  const override;
    bool next() override;
    bool prev() override;
    int overdue_count() const override;

    void repeat(int rating) override;
private:
    std::vector<Clip*> m_clips;
    std::vector<Clip*> m_showed_clips;
    int m_showing_clip_index = -1;
};

class RepeatingClipQueueV2 : public BaseClipQueue
{
public:
    RepeatingClipQueueV2(Library*, const std::vector<File*>&);
    RepeatingClipQueueV2(Library*, const std::vector<Clip*>&);
    ~RepeatingClipQueueV2();

    void remove() override;

    bool is_reviewing() const override;
    bool has_next() const override;
    bool has_prev()  const override;
    bool next() override;
    bool prev() override;
    int overdue_count() const override;

    void repeat(int rating) override;
private:
    using It = std::vector<Clip*>::const_iterator;
    using RIt = std::vector<Clip*>::const_reverse_iterator;
    It find_prev() const;
    It find_next() const;
    It m_show_it;
    It m_review_it;

    std::vector<Clip*> m_clips;
    //int m_showing_clip_index = 0;
    //int m_rewieved_clip_index = 0;
};

class AddingClipsQueue : public BaseClipQueue
{
public:
    AddingClipsQueue(Library*, File*, const srs::IFactory*);

    void set_clip_user_data(std::unique_ptr<ClipUserData>) override;
protected:
    const File* get_current_file() const override;
    File* get_current_file() override;
private:
    const srs::IFactory* m_srs_factory;
    File* m_file;
};

class WatchClipQueue : public BaseClipQueue
{
public:
    WatchClipQueue(Library*, Clip*);
};

#endif // !PROJECT_H

