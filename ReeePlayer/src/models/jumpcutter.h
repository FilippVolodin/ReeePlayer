class JumpCutterSettings
{
public:
    float get_volume_threshold() const;
    void set_volume_threshold(float);

    float get_silence_speed() const;
    void set_silence_speed(float);
    bool is_silence_skipping() const;

    int get_min_silence_interval() const;
    void set_min_silence_interval(int);

    int get_margin_before() const;
    void set_margin_before(int);

    int get_margin_after() const;
    void set_margin_after(int);
private:
    float m_volume_threshold = 0.02;
    float m_silence_speed = 0.0;
    int m_min_silence_interval = 300;
    int m_margin_before = 100;
    int m_margin_after = 100;
};

class JumpCutter
{
public:
    //JumpCutter();
    JumpCutter(const QString& filename);

    bool is_enabled() const;
    void set_enabled(bool);

    void read_volumes(const QString& filename);
    void read_vad(const QString& filename);
    void fragment();
    bool current_interval_is_loud(int t) const;
    int next_interval(int t) const;
    int next_interval_in_chunks(int ch) const;
    int rewind(int, int) const;
    const std::vector<uint8_t>& get_max_volumes() const;
    const std::vector<bool>& get_intervals() const;
    const std::vector<uint8_t>& get_voice_probs() const;

    std::shared_ptr<JumpCutterSettings> get_settings() const;
    void apply_settings(std::shared_ptr<JumpCutterSettings>);
private:
    bool m_enabled = true;
    std::vector<uint8_t> m_max_volume;
    std::vector<bool> m_fragments;
    std::vector<uint8_t> m_probs;
    std::shared_ptr<JumpCutterSettings> m_settings;
};

bool is_vol_exist(const QString&);
void create_vol_file(const QString& temp_wav, const QString& vol_filename, std::function<void(QString)> log);
QString get_vol_file(const QString&);
QString create_wav(const QString& filename, std::function<void(QString)> log);
std::vector<uint8_t> read_wav(const QString& filename, std::function<void(QString)> log);
void save_volumes(const QString& filename, const std::vector<uint8_t>&);
