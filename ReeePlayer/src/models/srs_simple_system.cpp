#include <pch.h>
#include <srs_simple_system.h>

using namespace std::chrono;

srs::simple::Card::Card(QJsonObject json)
{
    bool res = true;
    if (json.contains("level") && json["level"].isDouble())
        m_level = json["level"].toDouble();
    else
        res = false;
    
    if (json.contains("repeated") && json["repeated"].isDouble())
    {
        int64_t rep_ms = json["repeated"].toInteger();
        m_repeated = TimePoint(milliseconds(rep_ms));
    }
    else
        res = false;

    if (!res)
        throw ReadException();
}

void srs::simple::Card::repeat(TimePoint now, int rating)
{
}

srs::ICardUPtr srs::simple::Factory::create_card()
{
    return std::make_unique<Card>();
}
