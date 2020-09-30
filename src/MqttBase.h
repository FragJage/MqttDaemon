#ifndef MQTTBASE_H
#define MQTTBASE_H

#include <string>
#include "mqtt/client.h"

class MqttBase : public virtual mqtt::callback
{
    public:
	MqttBase();
	virtual ~MqttBase();

	void SetServer(const std::string& server, const std::string& id);
	void SetMainTopic(const std::string& mainTopic);
	void SetKeepAlive(int keepalive);
	void SetTimeout(int timeout);
	std::string GetMainTopic();
	int GetKeepAlive();

	void Connect();
	void Disconnect();
	void Publish(const std::string& sensor, const std::string& value);
	void Publish(const std::string& sensor, const std::string& value, int qos, bool retained);
	void Subscribe(const std::string& topic, int qos=0);
	void Unsubscribe(const std::string& topic);

	virtual void on_message(const std::string& topic, const std::string& message)=0;

    protected:

    private:
	void message_arrived(mqtt::const_message_ptr msg);
	std::string m_Server;
	std::string m_Id;
	std::string m_MainTopic;
	mqtt::client* m_MqttClient;
	mqtt::connect_options m_ConnOpts;
};

#endif // MQTTBASE_H
