#pragma once
#ifndef BAEDSCRASHLOGGER_H
#define BAEDSCRASHLOGGER_H
#include <string>
#include <stdexcept>
#include <sstream>

namespace baedsLogger {
	void monitor();
	void setCrashFolder(std::string name);
	void setCrashName(std::string name);

	void logSystem(std::string sys);
	void logJob(std::string job);
	void log(std::string out);
	void errLog(std::string out);
	std::string getLog();
};
	
class gameException : public std::runtime_error {
	public:
		gameException(const std::string& arg, const char* file, int line) :
			std::runtime_error(arg) {
			std::ostringstream o;
			o << file << " (line " << line << ") " << " - " << arg;
			message = o.str();
		}
		~gameException() throw() {}
		const char* what() const throw() {
			return message.c_str();
		}
	private:
		std::string message;
};

#define game_throw(arg) throw gameException(arg, __FILE__, __LINE__);
#endif 