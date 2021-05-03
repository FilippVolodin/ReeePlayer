#pragma once

#include "types.h"

namespace qsubs
{
    void skip_whispaces(QStringView str, qsizetype& pos);
    bool is_digit(QChar c);
    bool is_number(QStringView str);
    bool is_whitespace(QChar c);
    QChar current(QStringView, qsizetype pos);
    bool end_of_line(QStringView, qsizetype pos);
    QString next_non_empty_line(QTextStream&);
    QStringView remainder(QStringView str, qsizetype pos);
    QStringView trim(QStringView str);
    QStringView trim_left(QStringView str);
    QStringView trim_right(QStringView str);
    QVector<QStringView> split_on_spaces(QStringView str);
    QStringView collect_digits(QStringView str, qsizetype& pos);
    Timestamp collect_timestamp(QStringView str, qsizetype& pos);
}
