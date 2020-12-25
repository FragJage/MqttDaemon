#include <iostream>
#include "MqttBase.h"

using namespace std;

MqttBase::MqttBase() : m_Server("tcp://localhost:1883"), m_Id(""), m_MainTopic(""), m_MqttClient(nullptr)
{
    m_ConnOpts.set_automatic_reconnect(true);
	m_ConnOpts.set_clean_session(true);
	m_ConnOpts.set_keep_alive_interval(300);
	m_ConnOpts.set_connect_timeout(5);
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

void MqttBase::SetAuthentication(const string& user, const string& password)
{
	m_ConnOpts.set_user_name(user);
	m_ConnOpts.set_password(password);
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

void MqttBase::SetKeepAlive(int keepalive)
{
	m_ConnOpts.set_keep_alive_interval(keepalive);
}

void MqttBase::SetTimeout(int timeout)
{
	m_ConnOpts.set_connect_timeout(timeout);
}

int MqttBase::GetKeepAlive()
{
    return m_ConnOpts.get_keep_alive_interval().count();
}

void MqttBase::Connect()
{
	if (m_MqttClient!=nullptr) Disconnect();

	m_MqttClient = new mqtt::client(m_Server, m_Id);
	m_MqttClient->set_callback(*this);
	m_MqttClient->connect(m_ConnOpts);
}

void MqttBase::Disconnect()
{
	if (m_MqttClient==nullptr) return;

	if(m_MqttClient->is_connected())
    {
		m_MqttClient->disconnect();
    }

	delete m_MqttClient;
	m_MqttClient = nullptr;
}

void MqttBase::Publish(const string& sensor, const string& value)
{
    Publish(sensor, value, 0, false);
}

void MqttBase::Publish(const string& sensor, const string& value, int qos, bool retained)
{
    PublishTopic(m_MainTopic+sensor, value, qos, retained);
}

void MqttBase::PublishTopic(const string& topic, const string& value, int qos, bool retained)
{
	if (m_MqttClient==nullptr) Connect();
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
