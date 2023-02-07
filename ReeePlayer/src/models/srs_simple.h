#ifndef SRS_SIMPLE_H
#define SRS_SIMPLE_H

#include <srs_interfaces.h>

namespace srs::simple
{
    class Card : public srs::ICard
    {
    public:
        Card();

        void read(const QJsonObject&) override;
        void write(QJsonObject&) const override;

        float get_priority(TimePoint now) const override;
        bool is_due(TimePoint now) const override;
        void repeat(TimePoint now, int rating) override;
    private:
        float m_level = 0.0f;
        TimePoint m_repeated;
    };
}

#endif // !SRS_SIMPLE_H