#ifndef SRS_ICARD_H
#define SRS_ICARD_H

#include <time_types.h>

namespace srs
{

    using ReadException = std::exception;

    class IModel
    {
    public:
        virtual ~IModel() = default;

        virtual bool use_rating() const = 0;
        virtual int get_default_rating(int replays_count) const = 0;
    };

    class ICard
    {
    public:
        virtual ~ICard();

        virtual const IModel* get_model() const = 0;

        virtual void read(const QJsonObject&) = 0;
        virtual void write(QJsonObject&) const = 0;

        virtual float get_priority(TimePoint now) const = 0;
        virtual bool is_due(TimePoint now) const = 0;
        virtual TimePoint get_due_date() const = 0;
        virtual std::vector<Duration> get_due_intervals(TimePoint now) const = 0;
        virtual void repeat(TimePoint now, int rating) = 0;
    };

    using ICardUPtr = std::unique_ptr<ICard>;
}

#endif // !SRS_ICARD_H