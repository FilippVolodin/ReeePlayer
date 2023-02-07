#include "pch.h"
#include "repetition_model.h"

int round50(int val)
{
    return (val + 25) / 50 * 50;
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
