#ifndef PROJECT_H
#define PROJECT_H

class Clip;
class File;
class IClipStorage;

class App;
class Library;

struct ClipPriorityCmp
{
    ClipPriorityCmp(time_t cur_time);
    int64_t m_cur_time;
    bool operator()(const Clip* lhs, const Clip* rhs);
};

class Session : public QObject
{
    Q_OBJECT
public:
    Session(Library*, const std::vector<File*>&);
    ~Session();

    bool has_prev_clip() const;
    bool has_next_clip() const;
    Clip* get_prev_clip();
    Clip* get_next_clip();
    void remove_clip(Clip*);
    int remain_clips();

    bool load_clips();
    bool repeat(const std::vector<File*>&);

    int get_num_clips() const;
private:

    App* m_app;
    std::vector<Clip*> m_clips;
    std::vector<Clip*> m_showed_clips;
    int m_showing_clip_index = -1;

    Library* m_library;
};

#endif // !PROJECT_H

