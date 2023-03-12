#ifndef SRS_SIMPLE_H
#define SRS_SIMPLE_H

#include <srs_icard.h>

namespace srs::simple
{
    class Simple : public IModel
    {
    public:
        bool use_rating() const override;
        int get_default_rating(int replays_count) const override;
    };

    class Card : public srs::ICard
    {
    public:
        Card(const Simple*);

        const IModel* get_model() const override;

        void read(const QJsonObject&) override;
        void write(QJsonObject&) const override;

        float get_priority(TimePoint now) const override;
        bool is_due(TimePoint now) const override;
        void repeat(TimePoint now, int rating) override;
    private:
        const Simple* m_simple;
        float m_level = 0.0f;
        TimePoint m_repeated;
    };
}

#endif // !SRS_SIMPLE_H