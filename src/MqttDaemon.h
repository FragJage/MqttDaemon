#ifndef MQTTDAEMON_H
#define MQTTDAEMON_H

#define LOG_ENTER LOG_DEBUG(m_Log) << "*** Enter ***"
#define LOG_EXIT_OK LOG_DEBUG(m_Log) << "*** Exit OK ***"
#define LOG_EXIT_KO LOG_DEBUG(m_Log) << "*** Exit KO ***"

#include<map>
#ifdef WIN32
#include <WinSock2.h>		// To stop windows.h including winsock.h
#endif
#include "MqttBase.h"
#include "MqttLogger.h"
#include "Service.h"
#include "SimpleIni.h"
#include "SimpleLog.h"

struct MqttQueue
{
	MqttQueue(std::string topic, std::string msg) : Topic(topic), Message(msg) {};
	std::string Topic;
	std::string Message;
};

class MqttDaemon : public Service::IService, public IMqttLogPublisher, public MqttBase
{
    public:
		MqttDaemon(const std::string& topic, const std::string& configFileName);
        virtual ~MqttDaemon();

		virtual void DaemonConfigure(SimpleIni& iniFile) = 0;
		virtual int DaemonLoop(int argc, char* argv[]) = 0;
		virtual void IncomingMessage(const std::string& topic, const std::string& message)=0;
		int WaitFor(int timeout);
		void Publish(const std::string& sensor, const std::string& value);
		void PublishAsyncAdd(const std::string& sensor, const std::string& value);
		void PublishAsyncLog(const std::string& message);
		void PublishAsyncStart();

		int ServiceLoop(int argc, char* argv[]);
		void SetConfigfile(const std::string& configFile);

        static const int RESTART_MQTTDAEMON;

	protected:
		SimpleLog* m_Log;

    private:
		void SetLogLevel(const std::string& level, SimpleLog::DefaultFilter* filter);
		void SetLogDestination(const std::string& destination);
		void ReadParameters(int argc, char* argv[]);

		void Configure();
		void MqttConfigure(SimpleIni& iniFile);
		void LogConfigure(SimpleIni& iniFile);
		void MqttLogConfigure(SimpleIni& iniFile);
        void on_message(const std::string& topic, const std::string& message);
        void IncomingLoggerMessage(const std::string& command, const std::string& message);
        void SendMqttMessages();

		std::ofstream m_logStream;
		std::string m_logFile;
		SimpleLog m_SimpleLog;
		SimpleLog::DefaultWriter m_LogWriter;
		SimpleLog::DefaultFilter m_LogFilter;
		MqttLogger m_MqttLogWriter;
		SimpleLog::DefaultFilter m_MqttLogFilter;
		std::string m_LoggerTopic;

		std::string m_ConfigFilename;
		int m_MqttQos;
		bool m_MqttRetained;
		bool m_WithThread;
		std::mutex m_MqttQueueAccess;
		ServiceConditionVariable m_MqttQueueCond;
		std::queue<MqttQueue> m_MqttQueue;
};

#endif // MQTTDAEMON_H
