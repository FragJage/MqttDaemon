#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <algorithm>

#include "MqttDaemon.h"
#include "SimpleFolders.h"
#include "StringTools.h"


using namespace std;

const int MqttDaemon::RESTART_MQTTDAEMON = 32767;

MqttDaemon::MqttDaemon(const string& topic, const string& configFileName) : m_logFile(""), m_MqttQos(0), m_MqttRetained(true)
{
	m_Log = &m_SimpleLog;
	m_SimpleLog.SetFilter(&m_LogFilter);
	m_SimpleLog.SetWriter(&m_LogWriter);

	m_ConfigFilename = SimpleFolders::AddFile(SimpleFolders::GetFolder(SimpleFolders::FolderType::Configuration), configFileName, "conf");
	if (!SimpleFolders::FileExists(m_ConfigFilename)) m_ConfigFilename = "";

	SetMainTopic(topic);
	m_LoggerTopic = "Logger/"+topic;
	m_MqttLogWriter.SetPublisher(this);
}

MqttDaemon::~MqttDaemon()
{
	if (m_logStream) m_logStream.close();
}

void MqttDaemon::SetConfigfile(const string& configFile)
{
	m_ConfigFilename = configFile;
}

void MqttDaemon::SetLogLevel(const string& slevel, SimpleLog::DefaultFilter* filter)
{
	string level = slevel;

	std::transform(level.begin(), level.end(), level.begin(), ::toupper);

	if ((level == "1") || (level == "FATAL"))
	{
		filter->SetLevel(SimpleLog::LVL_FATAL);
		LOG_VERBOSE(m_Log) << "Set log level to Fatal";
	}
	else if ((level == "2") || (level == "ERROR"))
	{
		filter->SetLevel(SimpleLog::LVL_ERROR);
		LOG_VERBOSE(m_Log) << "Set log level to Error";
	}
	else if ((level == "3") || (level == "WARNING"))
	{
		filter->SetLevel(SimpleLog::LVL_WARNING);
		LOG_VERBOSE(m_Log) << "Set log level to Warning";
	}
	else if ((level == "4") || (level == "INFO"))
	{
		filter->SetLevel(SimpleLog::LVL_INFO);
		LOG_VERBOSE(m_Log) << "Set log level to Info";
	}
	else if ((level == "5") || (level == "DEBUG"))
	{
		filter->SetLevel(SimpleLog::LVL_DEBUG);
		LOG_VERBOSE(m_Log) << "Set log level to Debug";
	}
	else if ((level == "6") || (level == "VERBOSE"))
	{
		filter->SetLevel(SimpleLog::LVL_VERBOSE);
		LOG_VERBOSE(m_Log) << "Set log level to Verbose";
	}
	else if ((level == "7") || (level == "TRACE"))
	{
		filter->SetLevel(SimpleLog::LVL_TRACE);
		LOG_VERBOSE(m_Log) << "Set log level to Trace";
	}
	else
	{
		filter->SetLevel(SimpleLog::LVL_WARNING);
		LOG_WARNING(m_Log) << "Unknown debug level " << level << " (Possible values Fatal,Error,Warning,Info,Debug,Verbose,Trace)";
	}
}

void MqttDaemon::SetLogDestination(const std::string& sdestination)
{
	string destination = sdestination;

	std::transform(destination.begin(), destination.end(), destination.begin(), ::toupper);
	if (destination == "COUT")
	{
		m_LogWriter.SetStream(cout);
		LOG_VERBOSE(m_Log) << "Set log destination to std::cout";
	}
	else if (destination == "CERR")
	{
		m_LogWriter.SetStream(cerr);
		LOG_VERBOSE(m_Log) << "Set log destination to std::cerr";
	}
	else if (destination == "CLOG")
	{
		m_LogWriter.SetStream(clog);
		LOG_VERBOSE(m_Log) << "Set log destination to std::clog";
	}
	else
	{
		if (m_logStream) m_logStream.close();
		m_logStream.open(sdestination, ios_base::app);
		m_LogWriter.SetStream(m_logStream);
		LOG_VERBOSE(m_Log) << "Set log destination to file " << sdestination;
	}
}

void MqttDaemon::Configure()
{
	SimpleIni iniFile;

	if (m_ConfigFilename == "")
	{
		LOG_INFO(m_Log) << "No config file";
		return;
	}

	LOG_INFO(m_Log) << "Load config (file " << m_ConfigFilename << ")";

	iniFile.SetOptions(iniFile.Comment, ";");
	if (!iniFile.Load(m_ConfigFilename))
	{
		LOG_INFO(m_Log) << "Unable to open config file.";
		return;
	}

	LogConfigure(iniFile);
	MqttConfigure(iniFile);
	MqttLogConfigure(iniFile);
	DaemonConfigure(iniFile);
}

void MqttDaemon::MqttConfigure(SimpleIni& iniFile)
{
	string svalue;
	string id;
	int ivalue;

	svalue = iniFile.GetValue("mqtt", "server", "tcp://127.0.0.1:1883");
	id = iniFile.GetValue("mqtt", "id", "");
	LOG_VERBOSE(m_Log) << "Set mqtt server to " << svalue;
	SetServer(svalue, id);

	ivalue = iniFile.GetValue("mqtt", "keepalive", 300);
	SetKeepAlive(ivalue);
	LOG_VERBOSE(m_Log) << "Set mqtt keepalive to " << ivalue;

	ivalue = iniFile.GetValue("mqtt", "timeout", 5);
	SetTimeout(ivalue);
	LOG_VERBOSE(m_Log) << "Set mqtt timeout to " << ivalue;

	svalue = iniFile.GetValue("mqtt", "topic", "");
	SetMainTopic(svalue);
	LOG_VERBOSE(m_Log) << "Set mqtt topic to " << svalue;

	m_MqttQos = iniFile.GetValue("mqtt", "qos", 0);
	LOG_VERBOSE(m_Log) << "Set mqtt qos to " << m_MqttQos;

	svalue = iniFile.GetValue("mqtt", "retained", "true");
	m_MqttRetained = (StringTools::IsEqualCaseInsensitive(svalue, "true")||svalue=="1");
	LOG_VERBOSE(m_Log) << "Set mqtt retained to " << m_MqttRetained;

    svalue = iniFile.GetValue("mqtt", "user", "");
    string pwd = iniFile.GetValue("mqtt", "password", "");
    if(svalue!="")
    {
        SetAuthentication(svalue, pwd);
        LOG_VERBOSE(m_Log) << "Set mqtt authentication";
    }
}

void MqttDaemon::LogConfigure(SimpleIni& iniFile)
{
	string svalue;

	svalue = iniFile.GetValue("log", "level", "ERROR");
	SetLogLevel(svalue, &m_LogFilter);

	svalue = iniFile.GetValue("log", "destination", "CLOG");
	SetLogDestination(svalue);

	svalue = iniFile.GetValue("log", "module", "");
	if (svalue != "")
	{
		m_LogFilter.SetModule(svalue);
		LOG_VERBOSE(m_Log) << "Set log Module to " << svalue;
	}

	svalue = iniFile.GetValue("log", "function", "");
	if (svalue != "")
	{
		m_LogFilter.SetFunction(svalue);
		LOG_VERBOSE(m_Log) << "Set log Function to " << svalue;
	}
}

void MqttDaemon::MqttLogConfigure(SimpleIni& iniFile)
{
	string svalue;
	bool bvalue;

	svalue = iniFile.GetValue("mqttlog", "enable", "false");
	bvalue = (StringTools::IsEqualCaseInsensitive(svalue, "true")||svalue=="1");
	if(!bvalue)
    {
        LOG_VERBOSE(m_Log) << "Mqtt logger is disabled.";
        return;
    }
    LOG_VERBOSE(m_Log) << "Mqtt logger is enabled.";

	svalue = iniFile.GetValue("mqttlog", "topic", "");
	if (svalue != "")
	{
		m_LoggerTopic = svalue;
		LOG_VERBOSE(m_Log) << "Set mqttlog Topic to " << svalue;
	}

	svalue = iniFile.GetValue("mqttlog", "level", "ERROR");
	SetLogLevel(svalue, &m_MqttLogFilter);

	svalue = iniFile.GetValue("mqttlog", "module", "");
	if (svalue != "")
	{
		m_MqttLogFilter.SetModule(svalue);
		LOG_VERBOSE(m_Log) << "Set mqttlog Module to " << svalue;
	}

	svalue = iniFile.GetValue("mqttlog", "function", "");
	if (svalue != "")
	{
		m_MqttLogFilter.SetFunction(svalue);
		LOG_VERBOSE(m_Log) << "Set mqttlog Function to " << svalue;
	}

	m_SimpleLog.AddWriter(&m_MqttLogWriter, &m_MqttLogFilter);
}

void MqttDaemon::Publish(const string& sensor, const string& value)
{
    MqttBase::Publish(sensor, value, m_MqttQos, m_MqttRetained);
}

void MqttDaemon::ReadParameters(int argc, char* argv[])
{
	int i;
	char** arg;

	arg = argv; arg++;
	for (i = 1; i < argc; ++i, ++arg)
	{
		if ((strcmp("-f", *arg) == 0) || (strcmp("--configfile", *arg) == 0))
		{
			SetConfigfile(*(arg + 1));
		}
		else if ((strcmp("-l", *arg) == 0) || (strcmp("--loglevel", *arg) == 0))
		{
			SetLogLevel(*(arg + 1), &m_MqttLogFilter);
		}
		else if ((strcmp("-d", *arg) == 0) || (strcmp("--logdestination", *arg) == 0))
		{
			SetLogDestination(*(arg + 1));
		}
	}
}

int MqttDaemon::ServiceLoop(int argc, char* argv[])
{
    int ret;

	LOG_ENTER;
	ReadParameters(argc, argv);

	do
    {
        Configure();
        Connect();
        ret = DaemonLoop(argc, argv);
        Disconnect();
    } while(ret == MqttDaemon::RESTART_MQTTDAEMON);

	LOG_EXIT_OK;
    return ret;
}

int MqttDaemon::WaitFor(int timeout)
{
    int ret = Service::Get()->WaitFor({ m_MqttQueueCond }, 250);        //Values of ret : -1 -> Timeout, 0 -> Service status change, 1 -> Need send mqtt messages
    if(ret == 1) SendMqttMessages();
    return ret;
}

void MqttDaemon::PublishAsyncAdd(const string& sensor, const string& value)
{
    lock_guard<mutex> lock(m_MqttQueueAccess);
    m_MqttQueue.emplace(sensor, value);
}

void MqttDaemon::PublishAsyncLog(const string& message)
{
    lock_guard<mutex> lock(m_MqttQueueAccess);
    m_MqttQueue.emplace("", message);
}

void MqttDaemon::PublishAsyncStart()
{
    m_MqttQueueCond.notify_one();
}

void MqttDaemon::SendMqttMessages()
{
	lock_guard<mutex> lock(m_MqttQueueAccess);
	while (!m_MqttQueue.empty())
	{
		MqttQueue& mqttQueue = m_MqttQueue.front();
		if(mqttQueue.Topic=="")
        {
            MqttBase::PublishTopic(m_LoggerTopic, mqttQueue.Message, 0, false);
        }
        else
        {
            MqttBase::Publish(mqttQueue.Topic, mqttQueue.Message, m_MqttQos, m_MqttRetained);
            LOG_VERBOSE(m_Log) << "Send " << mqttQueue.Topic << " : " << mqttQueue.Message;
        }
		m_MqttQueue.pop();
	}
}

void MqttDaemon::on_message(const string& topic, const string& message)
{
    thread t(&MqttDaemon::IncomingMessage, this, topic, message);
    t.detach();
}
