class JumpCutterSettings
{
public:
    bool is_enabled() const;
    void set_enabled(bool);

    int get_non_voice_volume() const;
    void set_non_voice_volume(int);

    float get_voice_prob_th() const;
    void set_voice_prob_th(float);

    float get_non_voice_speed() const;
    void set_non_voice_speed(float);
    bool is_non_voice_skipping() const;

    int get_min_non_voice_interval() const;
    void set_min_non_voice_interval(int);

    int get_margin_before() const;
    void set_margin_before(int);

    int get_margin_after() const;
    void set_margin_after(int);
private:
    bool m_enabled = true;
    float m_voice_prob_th = 0.02;
    int m_non_voice_volume = 100;
    float m_non_voice_speed = 0.0;
    int m_min_non_voice_interval = 300;
    int m_margin_before = 100;
    int m_margin_after = 100;
};

class JumpCutter
{
public:
    JumpCutter(const QString& filename);

    void read_volumes(const QString& filename);
    const std::vector<uint8_t>& get_max_volumes() const;
private:
    std::vector<uint8_t> m_max_volume;
};

bool is_vol_exist(const QString&);
void create_vol_file(const QString& temp_wav, const QString& vol_filename, std::function<void(QString)> log);
QString get_vol_file(const QString&);
QString create_wav(const QString& filename, std::function<void(QString)> log);
std::vector<uint8_t> read_wav(QString filename, std::function<void(QString)> log);
void save_volumes(const QString& filename, const std::vector<uint8_t>&);
