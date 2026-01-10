#include "Timer.h"
#include <windows.h>

Timer::Timer()
{
    LARGE_INTEGER freq{};
    QueryPerformanceFrequency(&freq);
    mSecondsPerCount = 1.0 / static_cast<double>(freq.QuadPart);
}

void Timer::Reset()
{
    LARGE_INTEGER t{};
    QueryPerformanceCounter(&t);
    mBaseTime = t.QuadPart;
    mPrevTime = t.QuadPart;
    mCurrTime = t.QuadPart;
}

float Timer::Tick()
{
    LARGE_INTEGER t{};
    QueryPerformanceCounter(&t);
    mCurrTime = t.QuadPart;

    double dt = (mCurrTime - mPrevTime) * mSecondsPerCount;
    mPrevTime = mCurrTime;

    if (dt < 0.0) dt = 0.0;
    if (dt > 0.1) dt = 0.1; // îCà”ÅFÉfÉoÉbÉKí‚é~å„Ç»Ç«ÇÃã}ëùó}êß

    return static_cast<float>(dt);
}
