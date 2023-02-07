#ifndef SRS_FSRS_H
#define SRS_FSRS_H

#include <srs_interfaces.h>

namespace srs::fsrs
{

    enum class State { New, Learning, Review, Relearning };

    enum class Rating { Again, Hard, Good, Easy, COUNT };

    struct ReviewLog
    {
        ReviewLog();
        ReviewLog(
            Rating rating,
            int elapsed_days,
            int scheduled_days,
            TimePoint review,
            State state);

        Rating rating = Rating::Again;
        int elapsed_days = 0;
        int scheduled_days = 0;
        TimePoint review;
        State state = State::New;
    };

    struct CardPrivate
    {
        // Need to be stored
        float stability = 0;
        float difficulty = 0;
        int reps = 0;
        int lapses = 0;
        State state = State::New;
        TimePoint last_review;

        TimePoint due = now();

        // Doesn't need to be stored
        int elapsed_days = 0;
        int scheduled_days = 0;
    };

    struct SchedulingInfo
    {
        CardPrivate card;
        ReviewLog review_log;
    };

    using RecordLog = std::array<SchedulingInfo, static_cast<int>(Rating::COUNT)>;

    struct SchedulingCards
    {
        SchedulingCards(const CardPrivate& card);
        void update_state(State state);
        void schedule(TimePoint now,
            float hard_interval, float good_interval, float easy_interval);
        RecordLog record_log(const CardPrivate& card, TimePoint now);

        CardPrivate again;
        CardPrivate hard;
        CardPrivate good;
        CardPrivate easy;
    };

    constexpr int WEIGHT_COUNT = 13;

    struct Parameters
    {
        float request_retention = 0.9f;
        int maximum_interval = 36500;
        float easy_bonus = 1.3f;
        float hard_factor = 1.2f;
        std::array<float, WEIGHT_COUNT> w =
        { 1.f, 1.f, 5.f, -0.5f, -0.5f, 0.2f, 1.4f,
        -0.12f, 0.8f, 2.f, -0.2f, 0.2f, 1.f };
    };

    class FSRS
    {
    public:
        fsrs::RecordLog repeat(CardPrivate card, TimePoint now) const;
    private:
        void init_ds(SchedulingCards& s) const;
        void next_ds(SchedulingCards& s, float last_d, float last_s, float retrievability) const;
        float init_stability(Rating r) const;
        float init_difficulty(Rating r) const;
        int next_interval(float s) const;
        float next_difficulty(float d, Rating r) const;
        float mean_reversion(float init, float current) const;
        float next_recall_stability(float d, float s, float r) const;
        float next_forget_stability(float d, float s, float r) const;
        Parameters p;
    };

    class Card : public srs::ICard
    {
    public:
        Card(const FSRS&);

        void read(const QJsonObject&) override;
        void write(QJsonObject&) const override;

        float get_priority(TimePoint now) const override;
        bool is_due(TimePoint now) const override;
        void repeat(TimePoint now, int rating) override;
    private:
        const FSRS& m_fsrs;
        CardPrivate m_p;
    };
}

#endif // !SRS_FSRS_H