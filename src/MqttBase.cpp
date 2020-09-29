#include <iostream>

#include "MqttBase.h"

using namespace std;

MqttBase::MqttBase() : m_Server("tcp://localhost:1883"), m_Id(""), m_MainTopic(""), m_MqttCli(m_Server, "X"), m_MqttClient(&m_MqttCli)
{
	m_MqttConnOpts.set_automatic_reconnect(true);
	m_MqttConnOpts.set_clean_session(true);
    m_MqttConnOpts.set_connect_timeout(5);
	m_MqttConnOpts.set_keep_alive_interval(300);
}

MqttBase::~MqttBase()
{
	Disconnect();
}

void MqttBase::SetServer(const string& server, const string& id)
{
	m_Server = server;
	m_Id = id;
}

void MqttBase::SetMainTopic(const string& mainTopic)
{
    m_MainTopic = mainTopic;
    if(m_MainTopic.back() != '/') m_MainTopic.append("/");
}

string MqttBase::GetMainTopic()
{
    return m_MainTopic;
}

void MqttBase::SetKeepAlive(int keepAlive)
{
	m_MqttConnOpts.set_keep_alive_interval(keepAlive);
}

void MqttBase::SetTimeout(int timeout)
{
    m_MqttConnOpts.set_connect_timeout(timeout);
}

void MqttBase::SetAuthentication(const std::string& user, const std::string& password)
{
	m_MqttConnOpts.set_user_name(user);
	m_MqttConnOpts.set_password(password);
}

int MqttBase::GetKeepAlive()
{
	return m_MqttConnOpts.get_keep_alive_interval().count();
}

void MqttBase::Connect()
{
	//if (m_MqttClient!=nullptr)
	Disconnect();

	//m_MqttClient = new mqtt::client(m_Server, "X");
	m_MqttClient->set_callback(*this);
	m_MqttClient->connect(m_MqttConnOpts);
}

void MqttBase::Disconnect()
{
	//if (m_MqttClient==nullptr) return;

	if(m_MqttClient->is_connected())
		m_MqttClient->disconnect();

	//delete m_MqttClient;
	//m_MqttClient = nullptr;
}

void MqttBase::Publish(const string& sensor, const string& value)
{
	if (m_MqttClient==nullptr) Connect();

    string topic = m_MainTopic+sensor;
	m_MqttClient->publish(topic, value.c_str(), value.length());
}

void MqttBase::Publish(const string& sensor, const string& value, int qos, bool retained)
{
	if (m_MqttClient==nullptr) Connect();

    string topic = m_MainTopic+sensor;
	m_MqttClient->publish(topic, value.c_str(), value.length(), qos, retained);
}

void MqttBase::Subscribe(const string& topic, int qos)
{
	if (m_MqttClient == nullptr) Connect();

	m_MqttClient->subscribe(topic, qos);
}

void MqttBase::Unsubscribe(const std::string& topic)
{
	if (m_MqttClient == nullptr) return;
	m_MqttClient->unsubscribe(topic);
}

void MqttBase::message_arrived(mqtt::const_message_ptr msg)
{
	on_message(msg->get_topic(), msg->get_payload_str());
	return;
}
