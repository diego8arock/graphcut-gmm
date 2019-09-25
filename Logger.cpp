#include "Logger.h"
#include <iostream>

Logger* Logger::m_pInstance = NULL;

Logger* Logger::Instance()
{
	if (!m_pInstance)   
		m_pInstance = new Logger;

	return m_pInstance;
}

bool Logger::openLogFile(const char* logFile) {

	errno_t err;
	err = freopen_s(&file, logFile, "w", stderr);
	return err != 0 ? false : true;
}

bool Logger::closeLogFile() {

	fclose(file);
	return true;
}

void Logger::writeToLog(const char* message) {

	std::clog << formatMessage(message) << std::endl;

}

char* Logger::formatMessage(const char* message) {

	char result[500];
	strcpy_s(result, getFormatTime());
	strcat_s(result, ": ");
	strcat_s(result, message);
	return result;

}

char* Logger::getFormatTime() {

	time_t curr_time;
	tm curr_tm;
	time(&curr_time);
	localtime_s(&curr_tm, &curr_time);
	char time_string[100];
	strftime(time_string, 50, "[%T]", &curr_tm);
	return time_string;

}