#ifndef MQTTLOGGER_H
#define MQTTLOGGER_H
#include "SimpleLog.h"


class IMqttLogPublisher
{
	public:
		virtual void PublishAsyncLog(const std::string& message) = 0;
		virtual void PublishAsyncStart() = 0;
};

class MqttLogger : public SimpleLog::IWriter
{
	public:
		MqttLogger();
		virtual ~MqttLogger();
		bool IsActive();
		std::string GetFormattedText(SimpleLog::Level level, const std::string& message, const std::string& moduleName, int lineNumber, const std::string& functionName);
		void SetPublisher(IMqttLogPublisher* mqttPublisher);
		void Writer(SimpleLog::Level level, const std::string& message, const std::string& module, int Line, const std::string& function);
		void Flush();
    private:
        IMqttLogPublisher* m_Mqtt;
};
#endif // MQTTLOGGER_H
