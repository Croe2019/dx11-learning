#pragma once
#include <cstdint>

class Timer
{
public:
    Timer();

    void Reset();        // 計測開始
    float Tick();        // 前フレームからの経過秒を返す（dt）

    float DeltaTime() const { return mDeltaTime; }
    float TotalTime() const; // Resetからの総経過秒

private:
    double mSecondsPerCount = 0.0;
    int64_t mBaseTime = 0;
    int64_t mPrevTime = 0;
    int64_t mCurrTime = 0;

    float mDeltaTime = 0.0f;
};
