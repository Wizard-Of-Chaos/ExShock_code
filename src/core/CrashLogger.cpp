#include "CrashLogger.h"
#include <thread>
#include <mutex>
#include <filesystem>
#include <exception>
#include <Windows.h>
#include <stacktrace>
#include <fstream>
#include <iostream>
#include <csignal>

namespace baedsLogger 
{
	static std::thread out;
	static std::mutex mutex;
	static std::condition_variable condition;
	static std::stacktrace trace;

	enum class STATUS {
		STAT_RUNNING,
		STAT_CRASH,
		STAT_END,
		STAT_EXIT
	};

	static std::atomic<STATUS> programStatus = STATUS::STAT_RUNNING;

	static std::string outputFolder = "crashlogs/";
	static std::string outputName = "Crashlog_";

	static int crashSig = 0;

	static std::string currentSystem = "";
	static std::string currentJob = "";

	static std::mutex logMtx;
	static std::string logBuffer;

	void logSystem(std::string sys) {
		currentSystem = sys;
	}
	void logJob(std::string job) {
		currentJob = job;
	}

	static std::string _err(int err)
	{
		switch (err) {
		case SIGTERM:
			return "SIGTERM";
		case SIGSEGV:
			return "SIGSEGV";
		case SIGINT:
			return "SIGINT";
		case SIGILL:
			return "SIGILL";
		case SIGABRT:
			return "SIGABRT";
		case SIGFPE:
			return "SIGFPE";
		}
		return "";
	}
#define _CRT_SECURE_NO_WARNINGS

	std::string _fullLogStr()
	{
		std::ostringstream o;
		o << "System: " << currentSystem << std::endl << std::endl;
		o << trace << std::endl << std::endl;
		o << "Complete log: " << logBuffer;
		return o.str();
	}

	static void _output()
	{
		printf("Puking up a stack trace into file\n");
		std::string path = outputFolder;

		std::time_t time = std::time(nullptr);
		path += outputName + std::to_string(time) + ".txt";
		printf("Puking up a stack trace into file ");
		printf(path.c_str());
		printf("\n");

		std::error_code err;
		std::filesystem::create_directories(outputFolder, err);

		std::ofstream log;
		log.open(path);
		log << "Signal: " << crashSig << " (" << _err(crashSig) << ") " << std::endl;
		log << _fullLogStr();
		log.close();
	}

	static void _crashHandling()
	{
		if (programStatus != STATUS::STAT_RUNNING) return; // why are we crashing IN THE CRASH THING
		printf("Getting a stack trace\n");
		trace = std::stacktrace::current();
		programStatus = STATUS::STAT_CRASH;
		condition.notify_one();
		std::unique_lock<std::mutex> lock(mutex);
		condition.wait(lock, [] {return programStatus != STATUS::STAT_CRASH; });
	}

	static void _terminate()
	{
		_crashHandling();
		std::quick_exit(1);
	}

	static void _sigHandle(int sig)
	{
		crashSig = sig;
		_crashHandling();
		_terminate();
	}

	__declspec(noinline) static LONG WINAPI _exceptionHandler(EXCEPTION_POINTERS*)
	{
		_crashHandling();
		return EXCEPTION_CONTINUE_SEARCH;
	}

	static void __cdecl _invalid(const wchar_t*, const wchar_t*, const wchar_t*, unsigned int, uintptr_t) {
		_crashHandling();
		abort();
	}

	void _handleThread() {
		std::unique_lock<std::mutex> lock(mutex);
		printf("Thread for crash handling started\n");
		//wait on the program to not be running
		condition.wait(lock, [] {return programStatus != STATUS::STAT_RUNNING; });

		if (programStatus == STATUS::STAT_CRASH) {
			printf("Crash detected!\n");
			_output();
		}
		printf("Exiting program\n");
		programStatus = STATUS::STAT_END;
		condition.notify_one();
	}

	static void _normalExit()
	{
		programStatus = STATUS::STAT_EXIT;
		condition.notify_one();
		out.join();
	}

	void monitor()
	{
		printf("Monitoring for crashes\n");

		out = std::thread(_handleThread);

		SetUnhandledExceptionFilter(_exceptionHandler);
		std::signal(SIGABRT, _sigHandle);
		std::signal(SIGSEGV, _sigHandle);
		std::signal(SIGILL, _sigHandle);
		std::set_terminate(_terminate);
		_set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);
		_set_purecall_handler(_terminate);
		_set_invalid_parameter_handler(&_invalid);

		std::atexit(_normalExit);
	}

	void setCrashFolder(std::string name)
	{
		outputFolder = name;
	}

	void setCrashName(std::string name)
	{
		outputName = name;
	}

	void _logthrd(std::string out)
	{
		logMtx.lock();
		logBuffer += out;
		logMtx.unlock();
	}
	void _errlgthrd(std::string out)
	{
		logMtx.lock();
		logBuffer += "[ERROR] " + out;
		logMtx.unlock();
	}
#ifndef _DEBUG
	void log(std::string out)
	{
		std::jthread t(_logthrd, out);
	}
	void errLog(std::string out)
	{
		std::jthread t(_errlgthrd, out);
	}
#else
	void log(std::string out)
	{
		std::cout << out;
		std::jthread t(_logthrd, out);
	}
	void errLog(std::string out)
	{
		std::cerr << "[ERROR] " << out;
		std::jthread t(_errlgthrd, out);
	}
#endif
	std::string getLog() 
	{
		return _fullLogStr();
	}
}