#include <pch.h>
#include <srs.h>
#include <srs_simple.h>
#include <srs_fsrs.h>

srs::IFactory::~IFactory()
{
}

srs::Factory::Factory()
{
}

srs::ICardUPtr srs::Factory::create_card() const
{
    return std::make_unique<srs::fsrs::Card>(m_fsrs);
}

srs::ICardUPtr srs::Factory::create_card(const QString& type) const
{
    if (type == "simple")
        return std::make_unique<srs::simple::Card>();
    else if (type == "fsrs")
        return std::make_unique<srs::fsrs::Card>(m_fsrs);
    else
        return nullptr;
}
