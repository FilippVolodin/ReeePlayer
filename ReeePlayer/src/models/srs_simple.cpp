#include <pch.h>
#include <srs_simple.h>

using namespace std::chrono;

namespace srs::simple
{
    static Duration get_repetititon_interval(float level)
    {
        constexpr float BREAK_LEVEL1 = 1;
        constexpr float BASE1 = 10;
        constexpr float INT1 = 12 * 60;

        constexpr float BREAK_LEVEL2 = 3;
        constexpr float BASE2 = 2;
        static    float INT2 = INT1 * std::pow(BASE1, BREAK_LEVEL2 - BREAK_LEVEL1);

        constexpr float BREAK_LEVEL3 = 5;
        constexpr float BASE3 = 1.61803399f;
        static    float INT3 = INT2 * std::pow(BASE2, BREAK_LEVEL3 - BREAK_LEVEL2);

        int64_t dt;

        if (level < BREAK_LEVEL2)
            dt = INT1 * std::pow(BASE1, level - BREAK_LEVEL1);
        else if (level < BREAK_LEVEL3)
            dt = INT2 * std::pow(BASE2, level - BREAK_LEVEL2);
        else
            dt = INT3 * std::pow(BASE3, level - BREAK_LEVEL3);

        return Duration(dt);
    }

    float get_next_level(Duration elapsed, float level)
    {
        if (std::abs(level) < 0.00001)
            return 1.0;

        float next;
        Duration rep_int = get_repetititon_interval(level);
        int64_t rep_ms = rep_int.count();

        if (elapsed < rep_int)
        {
            float x = (float)elapsed.count() / rep_ms;

            float f = 1.0 / (1.0 + qExp((0.5 - x) * 10));
            f = (f - 0.00669285092) * 1.01356730981;
            next = level + f;
        }
        else
        {
            int64_t q_ms = rep_ms * 1.5 + 86400 * 1.5;
            float x = (float)(elapsed.count() - rep_ms) / (2 * q_ms);
            float val = (x + 1) / qExp(x);
            next = level * val + 1;
        }
        return next;
    }
}

bool srs::simple::Simple::use_rating() const
{
    return false;
}

int srs::simple::Simple::get_default_rating(int replays_count) const
{
    return 0;
}

srs::simple::Card::Card(const Simple* simple)
    : m_simple(simple)
{
}

const srs::IModel* srs::simple::Card::get_model() const
{
    return m_simple;
}

void srs::simple::Card::read(const QJsonObject& json)
{
    bool res = true;
    if (json.contains("level") && json["level"].isDouble())
        m_level = json["level"].toDouble();
    else
        res = false;

    if (json.contains("repeated") && json["repeated"].isDouble())
    {
        int64_t rep_sec = json["repeated"].toInteger();
        m_repeated = TimePoint(seconds(rep_sec));
    }
    else
        res = false;

    if (!res)
        throw ReadException();
}

void srs::simple::Card::write(QJsonObject& json) const
{
    json["type"] = "simple";
    json["level"] = m_level;
    json["repeated"] = m_repeated.time_since_epoch().count();
}

float srs::simple::Card::get_priority(TimePoint now) const
{
    if (std::abs(m_level) < 0.00001)
        return 1.0;

    Duration rep_int = get_repetititon_interval(m_level);

    Duration elapsed = (now - m_repeated) - rep_int;
    if (elapsed.count() >= 0)
    {
        return 1 / qExp(m_level);
    }

    elapsed = now - m_repeated;
    float dl = get_next_level(elapsed, m_level) - m_level;
    return dl - 1;
}

bool srs::simple::Card::is_due(TimePoint now) const
{
    Duration ri = get_repetititon_interval(m_level);
    return now >= (m_repeated + ri) || m_level < 0.001;
}

void srs::simple::Card::repeat(TimePoint now, int rating)
{
    m_level = get_next_level(now - m_repeated, m_level);
    m_repeated = now;
}
