#pragma once
#include "Engine/Commons/Callstack.hpp"
#include <thread>
#include "Engine/Core/Async/MPSCAsyncRingBuffer.hpp"
#include "Engine/Core/Async/Semaphores.hpp"
#include <set>
#include <shared_mutex>


//------------------------------------------------------------------------------------------------------------------------------
struct LogObject_T
{
	uint64_t			hpcTime;
	char*				filter;
	char*				line;
	Callstack			callstack;
};

//------------------------------------------------------------------------------------------------------------------------------
class LogSystem
{
public:
	LogSystem(const char* fileName);
	~LogSystem();

	std::thread			m_thread;
	std::string			m_filename;

	MPSCRingBuffer		m_messages;
	Semaphore			m_semaphore;

	std::ofstream*		m_fileStream = nullptr;

	bool				m_isRunning = true;

	void				LogSystemInit();
	void				LogSystemShutDown();
	void				WriteToLogFromBuffer(const LogObject_T& log);

	void				Logf(char const* filter, char const* format, ...);
	void				LogCallstackf(char const* filter, char const* format, ...);

	void				RunAllHooks(const LogObject_T* logObj);
	void				WaitForWork()			{ m_semaphore.Acquire(); }
	void				SignalWork()			{ m_semaphore.Release(1); }

	bool				CheckAgainstFilter(const char* filterToCheck);

	// Filtering
	void				LogEnableAll();  // all messages log
	void				LogDisableAll(); // not messages log
	void				LogEnable(char const* filter);     // this filter will start to appear in the log
	void				LogDisable(char const* filter);    // this filter will no longer appear in the log


	bool				IsRunning() const		{ return m_isRunning; }
	void				Stop()					{ m_isRunning = false; }
	void				LogFlush();
	bool				m_flushRequested = false;

	// NOTE - this message is only valid for the lifetime of the callback
	// and is not guaranteed to valid after your hook returns, so be sure to 
	// copy data you need!

	using LogHookCallback = void(*)(const LogObject_T*);
	
	void				LogHook(LogHookCallback cb);
	void				LogUnhook(LogHookCallback cb);


	bool					m_logSystemInitialized = false;

	std::vector<LogHookCallback>	m_logHooks;

	std::shared_mutex		m_filterMutex;
	std::set<std::string>	m_filterSet;
	bool					m_filterMode = true;	//If true, white-list(Enabled) all and ignore filters in the set, if false blacklist(Disable) all and only allow elements in the set
};