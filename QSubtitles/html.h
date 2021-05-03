#pragma once

namespace qsubs::html
{
    QString consume_char_ref(QStringView str, qsizetype& pos);
    QString consume_char_ref(QStringView str, qsizetype& pos, const std::set<QChar>& additional_allowed);
}