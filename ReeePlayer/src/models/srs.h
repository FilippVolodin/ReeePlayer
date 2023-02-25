#ifndef SRS_H
#define SRS_H

#include <srs_interfaces.h>
#include <srs_fsrs.h>

namespace srs
{
    class IModel
    {
    public:
        virtual ~IModel() = default;

        virtual int get_default_rating(int replays_count) const = 0;
    };

    class Model : public IModel
    {
    public:
        int get_default_rating(int replays_count) const override;
    };

    class IFactory
    {
    public:
        virtual ~IFactory() = default;

        virtual ICardUPtr create_card() const = 0;
        virtual ICardUPtr create_card(const QString& type) const = 0;

        virtual std::unique_ptr<IModel> create_model() const = 0;
    };

    class Factory : public IFactory
    {
    public:
        Factory();

        ICardUPtr create_card() const override;
        ICardUPtr create_card(const QString& type) const override;

        std::unique_ptr<IModel> create_model() const override;
    private:
        srs::fsrs::FSRS m_fsrs;
    };
}

#endif // !SRS_H