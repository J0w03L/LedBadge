#ifndef FRAMERATE_H
#define FRAMERATE_H

#include <chrono>
#include <iostream>
#include <ratio>
#include <thread>

template<std::intmax_t fps>
class frameRate
{
    public:
        frameRate():
            frameDelay{1},
            timePoint{std::chrono::steady_clock::now()}
        {}
        void sleep()
        {
            timePoint += frameDelay;
            std::this_thread::sleep_until(timePoint);
        }
    private:
        std::chrono::duration<double, std::ratio<1, fps>> frameDelay;
        std::chrono::time_point<std::chrono::steady_clock, decltype(frameDelay)> timePoint;
};

#endif // FRAMERATE_H
