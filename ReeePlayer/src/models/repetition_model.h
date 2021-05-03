#ifndef REPETITION_MODEL_H
#define REPETITION_MODEL_H

struct BestRepInterval
{
    int64_t begin;
    int64_t end;
};

int64_t now();
int round50(int val);
BestRepInterval get_repetititon_interval(float level);
float get_next_level(int64_t elapsed, float level);
float get_priority(int64_t now, int64_t time, float level);
QString get_interval_str(int64_t i);

#endif // !REPETITION_MODEL_H

