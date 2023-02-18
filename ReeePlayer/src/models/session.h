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
    class ICardFactory;
}

struct ClipPriorityCmp
{
    ClipPriorityCmp(TimePoint cur_time);
    TimePoint m_cur_time;
    bool operator()(const Clip* lhs, const Clip* rhs);
};

class IClipSession
{
public:
    virtual ~IClipSession();

    virtual const ClipUserData* get_clip_user_data() const = 0;
    virtual void set_clip_user_data(std::unique_ptr<ClipUserData>) = 0;
    virtual void remove() = 0;

    virtual const QString get_file_path() const = 0;
    virtual const FileUserData* get_file_user_data() const = 0;
    virtual void set_file_user_data(std::unique_ptr<FileUserData>) = 0;

    virtual bool has_next() const = 0;
    virtual bool has_prev() const = 0;
    virtual bool next() = 0;
    virtual bool prev() = 0;
    virtual int remain_count() const = 0;

    virtual void repeat(int rating) = 0;

    virtual void save_library() = 0;
};

class BaseClipSession : public IClipSession
{
public:
    BaseClipSession(Library*);

    const ClipUserData* get_clip_user_data() const override;
    void set_clip_user_data(std::unique_ptr<ClipUserData>) override;

    void remove() override;

    const QString get_file_path() const override;
    const FileUserData* get_file_user_data() const override;
    void set_file_user_data(std::unique_ptr<FileUserData>) override;

    bool has_next() const override;
    bool has_prev() const override;
    bool next() override;
    bool prev() override;
    int remain_count() const override;

    void repeat(int rating) override;

    void save_library() override;
protected:
    Clip* get_current_clip();
    const Clip* get_current_clip() const;
    void set_current_clip(Clip*);

    virtual const File* get_current_file() const;
    virtual File* get_current_file();
private:
    Library* m_library;
    Clip* m_current_clip = nullptr;
};

class RepeatingSession : public BaseClipSession
{
public:
    RepeatingSession(Library*, const std::vector<File*>&);
    RepeatingSession(Library*, const std::vector<Clip*>&);
    ~RepeatingSession();

    void remove() override;

    bool has_next() const override;
    bool has_prev()  const override;
    bool next() override;
    bool prev() override;
    int remain_count() const override;

    void repeat(int rating) override;
private:
    //App* m_app;
    std::vector<Clip*> m_clips;
    std::vector<Clip*> m_showed_clips;
    int m_showing_clip_index = -1;
};

class AddingClipsSession : public BaseClipSession
{
public:
    AddingClipsSession(Library*, File*, const srs::ICardFactory*);

    void set_clip_user_data(std::unique_ptr<ClipUserData>) override;
protected:
    const File* get_current_file() const override;
    File* get_current_file() override;
private:
    const srs::ICardFactory* m_factory;
    File* m_file;
};

class WatchClipSession : public BaseClipSession
{
public:
    WatchClipSession(Library*, Clip*);
};

class Session : public QObject
{
    Q_OBJECT
public:
    Session(Library*, const std::vector<File*>&);
    Session(Library*, const std::vector<Clip*>&);
    ~Session();

    bool has_prev_clip() const;
    bool has_next_clip() const;
    Clip* get_prev_clip();
    Clip* get_next_clip();
    void remove_clip(Clip*);
    int remain_clips();

    int get_num_clips() const;
private:
    std::vector<Clip*> m_clips;
    std::vector<Clip*> m_showed_clips;
    int m_showing_clip_index = -1;

    Library* m_library;
};

#endif // !PROJECT_H

