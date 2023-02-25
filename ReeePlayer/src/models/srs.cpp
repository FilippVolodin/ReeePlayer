#include <pch.h>
#include <srs.h>
#include <srs_simple.h>
#include <srs_fsrs.h>

int srs::Model::get_default_rating(int replays_count) const
{
    int rating = 4 - replays_count;
    if (rating < 0)
        rating = 0;
    else if (rating > 3)
        rating = 3;
    return rating;
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

std::unique_ptr<srs::IModel> srs::Factory::create_model() const
{
    return std::make_unique<Model>();
}
