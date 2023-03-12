#include <pch.h>
#include <srs.h>
#include <srs_simple.h>
#include <srs_fsrs.h>

srs::Factory::Factory()
    : m_simple_model(std::make_unique<simple::Simple>()),
    m_fsrs_model(std::make_unique<fsrs::FSRS>())
{
}

srs::ICardUPtr srs::Factory::create_card() const
{
    return std::make_unique<srs::fsrs::Card>(m_fsrs_model.get());
}

srs::ICardUPtr srs::Factory::create_card(const QString& type) const
{
    if (type == "simple")
        return std::make_unique<srs::simple::Card>(m_simple_model.get());
    else if (type == "fsrs")
        return std::make_unique<srs::fsrs::Card>(m_fsrs_model.get());
    else
        return nullptr;
}
