#include "pch.h"
#include "parser_utils.h"
#include "exception.h"

namespace qsubs
{

    void skip_whispaces(QStringView str, qsizetype& pos)
    {
        while (pos < str.size() && is_whitespace(str[pos]))
            ++pos;
    }

    bool is_digit(QChar c)
    {
        return c >= '0' && c <= '9';
    }

    bool is_number(QStringView str)
    {
        if (str.isEmpty())
            return false;
        
        for (QChar c : str)
        {
            if (!is_digit(c))
                return false;
        }
        return true;
    }

    bool is_whitespace(QChar c)
    {
        return
            c == QChar::Space ||
            c == QChar::Tabulation ||
            c == QChar::LineFeed ||
            c == QChar::FormFeed ||
            c == QChar::CarriageReturn;
    }

    QChar current(QStringView str, qsizetype pos)
    {
        return str[pos];
    }

    bool end_of_line(QStringView str, qsizetype pos)
    {
        return pos >= str.size();
    }

    QString next_non_empty_line(QTextStream& input)
    {
        QString res;
        while (!input.atEnd())
        {
            res = input.readLine();
            if (!res.isEmpty())
                break;
        }
        return res;
    }

    QStringView remainder(QStringView str, qsizetype pos)
    {
        return str.mid(pos);
    }

    QStringView trim(QStringView str)
    {
        return trim_left(trim_right(str));
    }

    QStringView trim_left(QStringView str)
    {
        int pos = 0;
        while (pos < str.length() && is_whitespace(str[pos]))
            ++pos;
        return str.mid(pos);
    }

    QStringView trim_right(QStringView str)
    {
        qsizetype pos = str.size() - 1;
        while (pos >= 0 && is_whitespace(str[pos]))
            --pos;
        return str.left(pos + 1);
    }

    QVector<QStringView> split_on_spaces(QStringView str)
    {
        QVector<QStringView> res;

        str = trim_left(str);

        qsizetype pos = 0;
        while (pos < str.length())
        {
            qsizetype start_pos = pos;
            while (pos < str.length() && !is_whitespace(str[pos]))
                ++pos;
            res.push_back(str.mid(start_pos, pos - start_pos));
            str = trim_left(str);
        }
        return res;
    }

    QStringView collect_digits(QStringView str, qsizetype& pos)
    {
        qsizetype start_pos = pos;
        while (!end_of_line(str, pos) && is_digit(current(str, pos)))
            ++pos;

        return str.mid(start_pos, pos - start_pos);
    }

    Timestamp collect_timestamp(QStringView str, qsizetype& pos)
    {
        enum class Units { Minutes, Hours };
        Units most_significant_inits = Units::Minutes;
        if (!is_digit(current(str, pos)))
            throw Exception();

        QStringView string = collect_digits(str, pos);
        int64_t value1 = string.toInt();
        if (string.length() != 2 || value1 > 59)
            most_significant_inits = Units::Hours;

        if (end_of_line(str, pos) || current(str, pos) != ':')
            throw Exception();

        ++pos;

        string = collect_digits(str, pos);
        if (string.length() != 2)
            throw Exception();

        int64_t value2 = string.toInt();
        int64_t value3;
        if (most_significant_inits == Units::Hours || !end_of_line(str, pos) && current(str, pos) == ':')
        {
            if (end_of_line(str, pos) || current(str, pos) != ':')
                throw Exception();

            ++pos;
            string = collect_digits(str, pos);
            if (string.length() != 2)
                throw Exception();

            value3 = string.toInt();
        }
        else
        {
            value3 = value2;
            value2 = value1;
            value1 = 0;
        }

        // comma is for SubRip
        if (end_of_line(str, pos) || !(current(str, pos) == '.' || current(str, pos) == ','))
            throw Exception();

        ++pos;

        string = collect_digits(str, pos);
        if (string.length() != 3)
            throw Exception();

        int64_t value4 = string.toInt();
        if (value2 > 59 || value3 > 59)
            throw Exception();

        return value1 * 3600000 + value2 * 60000 + value3 * 1000 + value4;
    }
}