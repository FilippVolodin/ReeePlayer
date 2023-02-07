#include <pch.h>
#include <srs.h>
#include <srs_simple.h>
#include <srs_fsrs.h>

srs::ICardFactory::~ICardFactory()
{
}

srs::CardFactory::CardFactory()
{
}

srs::ICardUPtr srs::CardFactory::create() const
{
    return std::make_unique<srs::fsrs::Card>(m_fsrs);
}

srs::ICardUPtr srs::CardFactory::create(const QString& type) const
{
    if (type == "simple")
        return std::make_unique<srs::simple::Card>();
    else if (type == "fsrs")
        return std::make_unique<srs::fsrs::Card>(m_fsrs);
    else
        return nullptr;
}
