#include "Timer.h"
#include <GLFW/glfw3.h>

Timer::Timer()
    : startTime(0)
    , elapsed(0)
    , running(false)
{
}

void Timer::Start() {
    if (!running) {
        startTime = glfwGetTime();
        running = true;
    }
}

void Timer::Stop() {
    if (running) {
        elapsed += glfwGetTime() - startTime;
        running = false;
    }
}

void Timer::Reset() {
    startTime = glfwGetTime();
    elapsed = 0;
    running = false;
}

float Timer::GetElapsed() const {
    if (running) {
        return elapsed + (glfwGetTime() - startTime);
    }
    return elapsed;
}