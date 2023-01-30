#ifndef SRS_ISYSTEM_H
#define SRS_ISYSTEM_H

namespace srs
{

    using TimePoint = std::chrono::time_point<std::chrono::system_clock>;

    using ReadException = std::exception;

    class ICard
    {
    public:
        virtual ~ICard();

        virtual void repeat(TimePoint now, int rating) = 0;
    };

    using ICardUPtr = std::unique_ptr<ICard>;

    class IFactory
    {
    public:
        virtual ~IFactory();

        virtual ICardUPtr create_card() = 0;
    };

}

#endif // !SRS_ISYSTEM_H