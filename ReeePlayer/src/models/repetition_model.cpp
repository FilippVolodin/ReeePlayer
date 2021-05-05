#include "pch.h"
#include "repetition_model.h"

int64_t now()
{
    using namespace std::chrono;
    return duration_cast<seconds>(
        system_clock::now().time_since_epoch()).count();
}

int round50(int val)
{
    return (val + 25) / 50 * 50;
}

BestRepInterval get_repetititon_interval(float level)
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

    BestRepInterval result;
    result.begin = dt;
    result.end = dt * 1.5 + 86400 * 1.5;
    return result;
}

float get_next_level(int64_t elapsed, float level)
{
    if (std::abs(level) < 0.00001)
        return 1.0;

    float next;
    BestRepInterval rep_int = get_repetititon_interval(level);

    if (elapsed < rep_int.begin)
    {
        float x = (float)elapsed / rep_int.begin;

        float f = 1.0 / (1.0 + qExp((0.5 - x) * 10));
        f = (f - 0.00669285092) * 1.01356730981;
        next = level + f;
    }
    else
    {
        constexpr float BASE3 = 1.61803399f;

        float x = (float)(elapsed - rep_int.begin) / (2 * rep_int.end);
        float val = (x + 1) / qExp(x);
        next = level * val + 1;
    }
    return next;
}

float get_priority(int64_t now, int64_t time, float level)
{
    if (std::abs(level) < 0.00001)
        return 1.0;

    BestRepInterval rep_int = get_repetititon_interval(level);

    int64_t elapsed = now - (time + rep_int.begin);
    if (elapsed >= 0)
    {
        return 1 / qExp(level);
    }

    elapsed = now - time;
    float dl = get_next_level(elapsed, level) - level;
    return dl - 1;
}

QString get_interval_str(int64_t interval)
{
    int64_t i = std::abs(interval);

    int seconds = i % 60;
    i /= 60;
    int minutes = i % 60;
    i /= 60;
    int hours = i % 24;
    i /= 24;
    int days = i % 30;
    i /= 30;
    int months = i % 12;
    i /= 12;
    int years = i;

    QString res;
    int unit = 0;
    if (years && unit <= 7)
    {
        res += QString::number(years) + " years ";
        if (!unit)
            unit = 6;
    }

    if (months && unit <= 6)
    {
        res += QString::number(months) + " months ";
        if (!unit)
            unit = 5;
    }

    if (days && unit <= 5)
    {
        res += QString::number(days) + " days ";
        if (!unit)
            unit = 4;
    }

    if (hours && unit <= 4)
    {
        res += QString::number(hours) + " hours ";
        if (!unit)
            unit = 3;
    }

    if (minutes && unit <= 3)
    {
        res += QString::number(minutes) + " minutes ";
        if (!unit)
            unit = 2;
    }

    if (seconds && unit <= 2)
    {
        res += QString::number(seconds) + " seconds ";
        if (!unit)
            unit = 1;
    }

    res = res.trimmed();
    if (interval > 0)
        res = "in " + res;
    else
        res = res + " ago";

    return res;
}
