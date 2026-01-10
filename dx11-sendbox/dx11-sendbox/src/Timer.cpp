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
    mDeltaTime = 0.0f;
}

float Timer::Tick()
{
    LARGE_INTEGER t{};
    QueryPerformanceCounter(&t);
    mCurrTime = t.QuadPart;

    double dt = (mCurrTime - mPrevTime) * mSecondsPerCount;
    mPrevTime = mCurrTime;

    // 異常値ガード（デバッガ停止後など）
    if (dt < 0.0) dt = 0.0;
    if (dt > 0.1) dt = 0.1; // 任意：極端なジャンプを抑える

    mDeltaTime = static_cast<float>(dt);
    return mDeltaTime;
}

float Timer::TotalTime() const
{
    return static_cast<float>((mCurrTime - mBaseTime) * mSecondsPerCount);
}
