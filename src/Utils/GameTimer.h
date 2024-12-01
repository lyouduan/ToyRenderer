#ifndef GAMETIMER_H
#define GAMETIMER_H

class GameTimer
{
public:
	GameTimer();

	float TotalTime() const;
	float DeltaTime() const;

	void Reset(); // call before message loop
	void Start(); // call when unpaused
	void Stop();  // Call when paused
	void Tick(); // call every frame

private:
	double mSecondsPerCount;
	double mDeltaTime;

	_int64 mBaseTime;
	_int64 mPausedTime;
	_int64 mStopTime; 
	_int64 mPrevTime; // previous time
	_int64 mCurrTime; // Current time

	bool mStopped;
};

#endif // GAMETIMER_H

