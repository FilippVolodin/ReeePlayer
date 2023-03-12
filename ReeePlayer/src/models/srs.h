#ifndef SRS_H
#define SRS_H

namespace srs
{
    class ICard;
    using ICardUPtr = std::unique_ptr<ICard>;
    
    namespace simple
    {
        class Simple;
    }

    namespace fsrs
    {
        class FSRS;
    }

    class IFactory
    {
    public:
        virtual ~IFactory() = default;

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
        std::unique_ptr<srs::simple::Simple> m_simple_model;
        std::unique_ptr<srs::fsrs::FSRS> m_fsrs_model;
    };
}

#endif // !SRS_H