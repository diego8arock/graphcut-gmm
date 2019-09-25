#ifndef LOGGER_H
#define LOGGER_H

#include <stddef.h>
#include <string>
#include <time.h>

class Logger {

public:
	static Logger* Instance();
	bool openLogFile(const char* logFile);
	bool closeLogFile();	
	void writeToLog(const char* message);	

private:
	Logger() {};  
	Logger(Logger const&) {};            
	Logger& operator=(Logger const&) {}; 
	static Logger* m_pInstance;
	char* getFormatTime();
	char* formatMessage(const char* message);
	FILE* file;
	
};

#endif