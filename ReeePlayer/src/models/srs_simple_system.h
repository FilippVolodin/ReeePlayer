#ifndef SRS_SIMPLE_SYSTEM_H
#define SRS_SIMPLE_SYSTEM_H

#include <srs_isystem.h>

namespace srs::simple
{
    class Card : public srs::ICard
    {
    public:
        Card(QJsonObject);

        void repeat(TimePoint now, int rating) override;
    private:
        float m_level = 0.0f;
        TimePoint m_repeated;
    };

    class Factory : public srs::IFactory
    {
    public:
        ICardUPtr create_card() override;

        ICardUPtr load_card_legacy();
    };
}

#endif