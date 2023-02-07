#ifndef SRS_INTERFACES_H
#define SRS_INTERFACES_H

#include <time_types.h>

namespace srs
{

    using ReadException = std::exception;

    class ICard
    {
    public:
        virtual ~ICard();

        virtual void read(const QJsonObject&) = 0;
        virtual void write(QJsonObject&) const = 0;

        virtual float get_priority(TimePoint now) const = 0;
        virtual bool is_due(TimePoint now) const = 0;
        virtual void repeat(TimePoint now, int rating) = 0;
    };

    using ICardUPtr = std::unique_ptr<ICard>;
}

#endif // !SRS_INTERFACES_H