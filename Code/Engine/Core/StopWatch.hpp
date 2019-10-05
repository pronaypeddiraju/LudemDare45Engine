//------------------------------------------------------------------------------------------------------------------------------
#pragma once
#include "Engine/Core/Clock.hpp"

typedef unsigned int uint;

//------------------------------------------------------------------------------------------------------------------------------
class StopWatch
{
public:
	StopWatch(Clock* clock);
	~StopWatch();

	void Start(float time);			// I prefer the name "Start" for this - sets the duration and resets the timer from the start; 
	void Set(float time);			// Sets our duration, but maintains current elapsed time
	void Reset();					// We have no elapsed time again

	float GetDuration() const;       // how long is this timer for total
	float GetElapsedTime() const;    // How long as the timer been running;
	float GetRemainingTime() const;  // how much time until it ends
	float GetNormalizedElapsedTime(); // 0 - 1 value saying how far I am into a duration (>1 if elapsed)

	bool HasElapsed();
	uint GetElapseCount();

	bool Decrement();          // remove one duration if we've elapsed (return whether it happened)
	uint DecrementAll();       // return number of "durations" passed, and remove them; 

public:
	Clock *m_clock; // Clock stopwatch is working off of (default to master)
	float m_startTime;
	float m_duration;

	// may need more for pause if you want a Pause & Resume
private:
	float m_elapsedTime = 0.f;
	int m_elapsedCount = 0;
};
