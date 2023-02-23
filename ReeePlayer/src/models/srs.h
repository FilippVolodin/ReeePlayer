#ifndef SRS_H
#define SRS_H

#include <srs_interfaces.h>
#include <srs_fsrs.h>

namespace srs
{
    class IFactory
    {
    public:
        virtual ~IFactory() = 0;

        virtual ICardUPtr create_card() const = 0;
        virtual ICardUPtr create_card(const QString& type) const = 0;


    };

    class Factory : public IFactory
    {
    public:
        Factory();

        ICardUPtr create_card() const override;
        ICardUPtr create_card(const QString& type) const override;
    private:
        srs::fsrs::FSRS m_fsrs;
    };
}

#endif // !SRS_H