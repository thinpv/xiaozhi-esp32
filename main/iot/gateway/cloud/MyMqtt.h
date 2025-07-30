#pragma once

#include <string>
#include "json.h"
#include "mqtt.h"
#include "ActionCallback.h"

#include <memory>

using namespace std;

class MyMqtt
{
private:
    string host;
    int port;
    const char *clientId;
    string username;
    string password;
    int keepalive;
    bool useTls;
    string willsetTopic;
    string willsetPayload;
    unique_ptr<Mqtt> mqtt_;

protected:
    vector<ActionCallback> actionCallbacks;

    virtual void OnConnect(int rc);
    virtual void OnDisconnect(int rc);
    virtual void OnPublish(int msgId);
    virtual void OnSubscribe(int msgId);
    virtual void OnSubscribeDone();
    virtual void OnUnsubscribe(int msgId);
    virtual void OnMessage(string topic, char *payload, int payloadLen);

public:
    MyMqtt(string host, int port, const char *clientId, string username, string password, int keepalive, bool useTls = false, string willsetTopic = "", string willsetPayload = "");

    int init();

    int Connect(int timeout = 10);
	int Subscribe(int *msgId, string topic);
	int Unsubscribe(int *msgId, string topic);
	int Publish(string topic, const char *payload, int payloadLen);
	int Publish(string topic, string payload);
	bool IsConnected();

    void AddActionCallback(ActionCallbackFuncType1 actionCallbackFuncType1, string topic);
    void AddActionCallback(ActionCallbackFuncType2 actionCallbackFuncType2, string topic);
    void AddActionCallback(ActionCallbackFuncType3 actionCallbackFuncType3, string topic);
    void AddActionCallback(ActionCallbackFuncType4 actionCallbackFuncType4, string topic);
    int FindActionCallbackFuncFromTopic(string topic, ActionCallback **actionCallback);
    void SubscribeList();

    bool CheckData(Json::Value &dataValue, bool &rs);
};
