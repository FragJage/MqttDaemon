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
	m_SimpleLog.SetFilter(&m_logFilter);
	m_SimpleLog.SetWriter(&m_logWriter);

	m_ConfigFilename = SimpleFolders::AddFile(SimpleFolders::GetFolder(SimpleFolders::FolderType::Configuration), configFileName, "conf");
	if (!SimpleFolders::FileExists(m_ConfigFilename)) m_ConfigFilename = "";

	SetMainTopic(topic);
}

MqttDaemon::~MqttDaemon()
{
	if (m_logStream) m_logStream.close();
}

void MqttDaemon::SetConfigfile(const string& configFile)
{
	m_ConfigFilename = configFile;
}

void MqttDaemon::SetLogLevel(const string& slevel)
{
	string level = slevel;

	std::transform(level.begin(), level.end(), level.begin(), ::toupper);

	if ((level == "1") || (level == "FATAL"))
	{
		m_logFilter.SetLevel(SimpleLog::LVL_FATAL);
		LOG_VERBOSE(m_Log) << "Set log level to Fatal";
	}
	else if ((level == "2") || (level == "ERROR"))
	{
		m_logFilter.SetLevel(SimpleLog::LVL_ERROR);
		LOG_VERBOSE(m_Log) << "Set log level to Error";
	}
	else if ((level == "3") || (level == "WARNING"))
	{
		m_logFilter.SetLevel(SimpleLog::LVL_WARNING);
		LOG_VERBOSE(m_Log) << "Set log level to Warning";
	}
	else if ((level == "4") || (level == "INFO"))
	{
		m_logFilter.SetLevel(SimpleLog::LVL_INFO);
		LOG_VERBOSE(m_Log) << "Set log level to Info";
	}
	else if ((level == "5") || (level == "DEBUG"))
	{
		m_logFilter.SetLevel(SimpleLog::LVL_DEBUG);
		LOG_VERBOSE(m_Log) << "Set log level to Debug";
	}
	else if ((level == "6") || (level == "VERBOSE"))
	{
		m_logFilter.SetLevel(SimpleLog::LVL_VERBOSE);
		LOG_VERBOSE(m_Log) << "Set log level to Verbose";
	}
	else if ((level == "7") || (level == "TRACE"))
	{
		m_logFilter.SetLevel(SimpleLog::LVL_TRACE);
		LOG_VERBOSE(m_Log) << "Set log level to Trace";
	}
	else
	{
		m_logFilter.SetLevel(SimpleLog::LVL_WARNING);
		LOG_WARNING(m_Log) << "Unknown debug level " << level << " (Possible values Fatal,Error,Warning,Info,Debug,Verbose,Trace)";
	}
}

void MqttDaemon::SetLogDestination(const std::string& sdestination)
{
	string destination = sdestination;

	std::transform(destination.begin(), destination.end(), destination.begin(), ::toupper);
	if (destination == "COUT")
	{
		m_logWriter.SetStream(cout);
		LOG_VERBOSE(m_Log) << "Set log destination to std::cout";
	}
	else if (destination == "CERR")
	{
		m_logWriter.SetStream(cerr);
		LOG_VERBOSE(m_Log) << "Set log destination to std::cerr";
	}
	else if (destination == "CLOG")
	{
		m_logWriter.SetStream(clog);
		LOG_VERBOSE(m_Log) << "Set log destination to std::clog";
	}
	else
	{
		if (m_logStream) m_logStream.close();
		m_logStream.open(sdestination, ios_base::app);
		m_logWriter.SetStream(m_logStream);
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
	DaemonConfigure(iniFile);
}

void MqttDaemon::MqttConfigure(SimpleIni& iniFile)
{
	string svalue;
	string id;
	int ivalue;

	svalue = iniFile.GetValue("mqtt", "server", "tcp://127.0.0.1:1883");
	id = iniFile.GetValue("mqtt", "id", "");
	LOG_VERBOSE(m_Log) << "Connecting to mqtt server " << svalue;
	SetServer(svalue, id);
	LOG_VERBOSE(m_Log) << "Mqtt server connected";

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
	SetLogLevel(svalue);

	svalue = iniFile.GetValue("log", "destination", "CLOG");
	SetLogDestination(svalue);

	svalue = iniFile.GetValue("log", "module", "");
	if (svalue != "")
	{
		m_logFilter.SetModule(svalue);
		LOG_VERBOSE(m_Log) << "Set log Module to " << svalue;
	}

	svalue = iniFile.GetValue("log", "function", "");
	if (svalue != "")
	{
		m_logFilter.SetFunction(svalue);
		LOG_VERBOSE(m_Log) << "Set log Function to " << svalue;
	}
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
			SetLogLevel(*(arg + 1));
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
