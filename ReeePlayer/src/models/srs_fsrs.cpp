#include <pch.h>
#include <srs_fsrs.h>

using namespace std::chrono;

// Rating to int conversion
static int operator+(srs::fsrs::Rating rating)
{
    return static_cast<int>(rating);
}

// State to int conversion
static int operator+(srs::fsrs::State state)
{
    return static_cast<int>(state);
}

srs::fsrs::ReviewLog::ReviewLog()
{
}

srs::fsrs::ReviewLog::ReviewLog(srs::fsrs::Rating rating, int elapsed_days, int scheduled_days,
    TimePoint review, State state)
    : rating(rating), elapsed_days(elapsed_days), scheduled_days(scheduled_days),
    review(review), state(state)
{
}

srs::fsrs::SchedulingCards::SchedulingCards(const CardPrivate& card)
    : again(card), hard(card), good(card), easy(card)
{
}

void srs::fsrs::SchedulingCards::update_state(State state)
{
    switch (state)
    {
    case State::New:
        again.state = State::Learning;
        hard.state = State::Learning;
        good.state = State::Learning;
        easy.state = State::Review;
        again.lapses += 1;
        break;
    case State::Learning:
    case State::Relearning:
        again.state = state;
        hard.state = State::Review;
        good.state = State::Review;
        easy.state = State::Review;
        break;
    case State::Review:
        hard.state = State::Review;
        good.state = State::Review;
        easy.state = State::Review;
        again.lapses += 1;
        break;
    default:
        break;
    }
}

void srs::fsrs::SchedulingCards::schedule(
    TimePoint now, float hard_interval, float good_interval, float easy_interval)
{
    again.scheduled_days = 0;
    hard.scheduled_days = hard_interval;
    good.scheduled_days = good_interval;
    easy.scheduled_days = easy_interval;
    again.due = now + std::chrono::minutes(5);
    hard.due = now + std::chrono::days(static_cast<int>(hard_interval));
    good.due = now + std::chrono::days(static_cast<int>(good_interval));
    easy.due = now + std::chrono::days(static_cast<int>(easy_interval));
}

srs::fsrs::RecordLog srs::fsrs::SchedulingCards::record_log(
    const CardPrivate& card, TimePoint now)
{
    return {
        SchedulingInfo(again,
            ReviewLog(Rating::Again, again.scheduled_days,
                      card.elapsed_days, now, card.state)),
        SchedulingInfo(hard,
            ReviewLog(Rating::Hard, hard.scheduled_days,
                      card.elapsed_days, now, card.state)),
        SchedulingInfo(good,
            ReviewLog(Rating::Good, good.scheduled_days,
                      card.elapsed_days, now, card.state)),
        SchedulingInfo(easy,
            ReviewLog(Rating::Easy, easy.scheduled_days,
                      card.elapsed_days, now, card.state))
    };
}

srs::fsrs::RecordLog srs::fsrs::FSRS::repeat(CardPrivate card, TimePoint now) const
{
    using namespace std::chrono;

    if (card.state == State::New)
        card.elapsed_days = 0;
    else
        card.elapsed_days = duration_cast<days>(now - card.last_review).count();

    card.last_review = now;
    card.reps += 1;
    SchedulingCards s(card);
    s.update_state(card.state);

    switch (card.state)
    {
    case State::New:
    {
        init_ds(s);
        s.again.due = now + minutes(1);
        s.hard.due = now + minutes(5);
        s.good.due = now + minutes(10);
        int easy_interval = next_interval(s.easy.stability * p.easy_bonus);
        s.easy.scheduled_days = easy_interval;
        s.easy.due = now + days(easy_interval);
        break;
    }
    case State::Learning:
    case State::Relearning:
    {
        int hard_interval = next_interval(s.hard.stability);
        int good_interval = std::max(
            next_interval(s.good.stability),
            hard_interval + 1);
        int easy_interval = std::max(
            next_interval(s.easy.stability * p.easy_bonus),
            good_interval + 1);
        s.schedule(now, hard_interval, good_interval, easy_interval);
        break;
    }
    case State::Review:
    {
        int interval = card.elapsed_days;
        float last_d = card.difficulty;
        float last_s = card.stability;
        float retrievability = std::exp(std::log(0.9f) * interval / last_s);
        next_ds(s, last_d, last_s, retrievability);

        int hard_interval = next_interval(last_s * p.hard_factor);
        int good_interval = next_interval(s.good.stability);
        hard_interval = std::min(hard_interval, good_interval);
        good_interval = std::max(good_interval, hard_interval + 1);
        int easy_interval = std::max(
            next_interval(s.easy.stability * p.hard_factor),
            good_interval + 1);
        s.schedule(now, hard_interval, good_interval, easy_interval);
        break;
    }
    default:
        break;
    }
    return s.record_log(card, now);
}

void srs::fsrs::FSRS::init_ds(SchedulingCards& s) const
{
    s.again.difficulty = init_difficulty(Rating::Again);
    s.again.stability = init_stability(Rating::Again);
    s.hard.difficulty = init_difficulty(Rating::Hard);
    s.hard.stability = init_stability(Rating::Hard);
    s.good.difficulty = init_difficulty(Rating::Good);
    s.good.stability = init_stability(Rating::Good);
    s.easy.difficulty = init_difficulty(Rating::Easy);
    s.easy.stability = init_stability(Rating::Easy);
}

void srs::fsrs::FSRS::next_ds(SchedulingCards& s, float last_d, float last_s, float retrievability) const
{
    s.again.difficulty = next_difficulty(last_d, Rating::Again);
    s.again.stability = next_forget_stability(
        s.again.difficulty, last_s, retrievability);
    s.hard.difficulty = next_difficulty(last_d, Rating::Hard);
    s.hard.stability = next_forget_stability(
        s.hard.difficulty, last_s, retrievability);
    s.good.difficulty = next_difficulty(last_d, Rating::Good);
    s.good.stability = next_forget_stability(
        s.good.difficulty, last_s, retrievability);
    s.easy.difficulty = next_difficulty(last_d, Rating::Easy);
    s.easy.stability = next_forget_stability(
        s.easy.difficulty, last_s, retrievability);
}

float srs::fsrs::FSRS::init_stability(Rating r) const
{
    return std::max(p.w[0] + p.w[1] * +r, 0.1f);
}

float srs::fsrs::FSRS::init_difficulty(Rating r) const
{
    return std::min(std::max(p.w[2] + p.w[3] * (+r - 2), 1.f), 10.f);
}

int srs::fsrs::FSRS::next_interval(float s) const
{
    float new_interval = s * std::log(p.request_retention) / std::log(0.9f);
    int new_interval_i = std::lround(new_interval);
    return std::min(std::max(new_interval_i, 1), p.maximum_interval);
}

float srs::fsrs::FSRS::next_difficulty(float d, Rating r) const
{
    float next_d = d + p.w[4] * (+r - 2);
    return std::min(std::max(mean_reversion(p.w[2], next_d), 1.f), 10.f);
}

float srs::fsrs::FSRS::mean_reversion(float init, float current) const
{
    return p.w[5] * init + (1 - p.w[5]) * current;
}

float srs::fsrs::FSRS::next_recall_stability(float d, float s, float r) const
{
    return s * (
        1 + std::exp(p.w[6]) *
        (11 - d) *
        std::pow(s, p.w[7]) *
        (std::exp((1 - r) * p.w[8]) - 1));
}

float srs::fsrs::FSRS::next_forget_stability(float d, float s, float r) const
{
    return p.w[9] *
        std::pow(d, p.w[10]) *
        std::pow(s, p.w[11]) *
        std::exp((1 - r) *
            p.w[12]);
}

srs::fsrs::Card::Card(const FSRS& fsrs) : m_fsrs(fsrs)
{

}

void srs::fsrs::Card::read(const QJsonObject& json)
{
    bool res = true;
    if (json.contains("stability") && json["stability"].isDouble())
        m_p.stability = json["stability"].toDouble();
    else
        res = false;

    if (json.contains("difficulty") && json["difficulty"].isDouble())
        m_p.difficulty = json["difficulty"].toDouble();
    else
        res = false;

    if (json.contains("reps") && json["reps"].isDouble())
        m_p.reps = json["reps"].toInt();
    else
        res = false;

    if (json.contains("lapses") && json["lapses"].isDouble())
        m_p.lapses = json["lapses"].toInt();
    else
        res = false;

    if (json.contains("state") && json["state"].isDouble())
        m_p.state = static_cast<State>(json["state"].toInt());
    else
        res = false;

    if (json.contains("last_review") && json["last_review"].isDouble())
    {
        int64_t last_review_sec = json["last_review"].toInteger();
        m_p.last_review = TimePoint(seconds(last_review_sec));
    }
    else
        res = false;

    if (json.contains("due") && json["due"].isDouble())
    {
        int64_t due_sec = json["due"].toInteger();
        m_p.due = TimePoint(seconds(due_sec));
    }

    if (!res)
        throw ReadException();
}

void srs::fsrs::Card::write(QJsonObject& json) const
{
    json["type"] = "fsrs";
    json["stability"] = m_p.stability;
    json["difficulty"] = m_p.difficulty;
    json["reps"] = m_p.reps;
    json["lapses"] = m_p.lapses;
    json["state"] = +m_p.state;
    json["last_review"] = m_p.last_review.time_since_epoch().count();
    json["due"] = m_p.due.time_since_epoch().count();
}

float srs::fsrs::Card::get_priority(TimePoint now) const
{
    return 0;
}

bool srs::fsrs::Card::is_due(TimePoint now) const
{
    return true;
}

void srs::fsrs::Card::repeat(TimePoint now, int rating)
{
    RecordLog record_log = m_fsrs.repeat(m_p, now);
    if (rating < 0)
        rating = 0;
    else if (rating >= +Rating::COUNT)
        rating = +Rating::COUNT - 1;

    m_p = record_log[rating].card;
}
