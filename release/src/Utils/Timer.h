#ifndef TIMER_H
#define TIMER_H

class Timer {
public:
    Timer();
    void Start();
    void Stop();
    void Reset();
    float GetElapsed() const;
    bool IsRunning() const { return running; }
    
private:
    float startTime;
    float elapsed;
    bool running;
};

#endif