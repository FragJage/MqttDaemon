#include <sstream>
#include <iomanip>
#include "MqttLogger.h"

using namespace std;

MqttLogger::MqttLogger() : m_Mqtt(nullptr)
{
}

MqttLogger::~MqttLogger()
{
    Flush();
    m_Mqtt = nullptr;
}

void MqttLogger::SetPublisher(IMqttLogPublisher* mqttPublisher)
{
    m_Mqtt = mqttPublisher;
}

void MqttLogger::Writer(SimpleLog::Level level, const string& message, const string& module, int line, const string& function)
{
   	ostringstream oss;
	time_t internToday = time(0);
	struct tm localToday;

	#ifdef _MSC_VER
    localtime_s(&localToday, &internToday);
	#else
    localToday = *localtime(&internToday);
	#endif

	oss << localToday.tm_year + 1900 << '-' << setw(2) << setfill('0') << localToday.tm_mon + 1 << '-' << setw(2) << setfill('0') << localToday.tm_mday << ' ';
	oss << setw(2) << setfill('0') << localToday.tm_hour << ':' << setw(2) << setfill('0') << localToday.tm_min << ':' << setw(2) << setfill('0') << localToday.tm_sec << ' ';
	oss << SimpleLog::Level2String(level) << " " << module << " l." << line << " " << function << " " << message << endl;

    if(m_Mqtt != nullptr)
    {
        m_Mqtt->PublishAsyncLog(oss.str());
    }
}

void MqttLogger::Flush()
{
    if(m_Mqtt != nullptr) m_Mqtt->PublishAsyncStart();
}
