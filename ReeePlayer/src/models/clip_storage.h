#ifndef ICLIP_H
#define ICLIP_H

class File;
class Library;

class Clip
{
    friend File;

    File* m_file = nullptr;
    std::time_t m_begin = 0;
    std::time_t m_end = 0;
    std::vector<QString> m_subtitles;
    float m_level = 0.0f;
    std::time_t m_added = 0;
    std::time_t m_rep_time = 0;
    std::vector<std::time_t> m_repeats;
    bool m_is_favorite = false;

    void set_file(File*);
public:
    Clip();

    const File* get_file() const;
    File* get_file();

    std::time_t get_begin() const;
    void set_begin(std::time_t);
    std::time_t get_end() const;
    void set_end(std::time_t);

    std::time_t get_adding_time() const;
    void set_adding_time(std::time_t);

    std::time_t get_rep_time() const;
    void set_rep_time(std::time_t);

    void add_repeat(std::time_t);
    const std::vector<std::time_t>& get_repeats() const;
    void set_repeats(std::vector<std::time_t>);

    float get_level() const;
    void set_level(float);

    QString get_subtitle(int index) const;
    const std::vector<QString>& get_subtitles() const;
    void set_subtitles(std::vector<QString>&&);

    bool is_favorite() const;
    void set_favorite(bool);
};

struct ClipCmp
{
    bool operator()(const Clip* l, const Clip* r) const
    {
        return l->get_begin() < r->get_begin();
    };
};

using Clips = std::multiset<Clip*, ClipCmp>;

class File
{
    QString m_path;
    Clips m_clips;
    Library* m_library = nullptr;
    int m_player_time = 0;
    int m_length = 0;
public:
    File(Library*, const QString& path);
    ~File();

    Library* get_library();

    QString get_path() const;

    int get_num_clips() const;
    const Clips& get_clips() const;

    void add_clip(Clip*);
    void remove_clip(Clip*);

    int get_player_time() const;
    void set_player_time(int);

    int get_length() const;
    void set_length(int);
};

class IClipStorage
{
public:
    virtual std::vector<Clip*> get_all_clips() const = 0;
    virtual std::vector<Clip*> get_all_repeatable_clips() const = 0;
    virtual std::vector<Clip*> find(QStringView, int max) const = 0;
};

#endif // !ICLIP_H

