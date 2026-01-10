#pragma once
#include <cstdint>

class Timer
{
public:
    Timer();

    void Reset();
    float Tick(); // dti•bj

private:
    double  mSecondsPerCount = 0.0;
    int64_t mBaseTime = 0;
    int64_t mPrevTime = 0;
    int64_t mCurrTime = 0;
};
