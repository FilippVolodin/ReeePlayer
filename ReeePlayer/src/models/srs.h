#ifndef SRS_H
#define SRS_H

#include <srs_interfaces.h>
#include <srs_fsrs.h>

namespace srs
{
    class ICardFactory
    {
    public:
        virtual ~ICardFactory() = 0;

        virtual ICardUPtr create() const = 0;
        virtual ICardUPtr create(const QString& type) const = 0;
    };

    class CardFactory : public ICardFactory
    {
    public:
        CardFactory();

        ICardUPtr create() const override;
        ICardUPtr create(const QString& type) const override;
    private:
        srs::fsrs::FSRS m_fsrs;
    };
}

#endif // !SRS_H