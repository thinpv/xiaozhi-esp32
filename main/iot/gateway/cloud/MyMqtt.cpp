#include <stdio.h>
#include "MyMqtt.h"
#include "Log.h"
#include "ErrorCode.h"
#include "Gateway.h"
// #include "Config.h"
#include "DeviceManager.h"
#include "Util.h"
#include "board.h"

MyMqtt::MyMqtt(string host,
               int port,
               const char *clientId,
               string username,
               string password,
               int keepalive,
               bool useTls,
               string willsetTopic,
               string willsetPayload) : host(host),
                                        port(port),
                                        clientId(clientId),
                                        username(username),
                                        password(password),
                                        keepalive(keepalive),
                                        useTls(useTls),
                                        willsetTopic(willsetTopic),
                                        willsetPayload(willsetPayload)
{
}

int MyMqtt::init()
{
    if (mqtt_ != nullptr)
    {
        LOGW("Mqtt client already started");
        mqtt_.reset();
    }

    auto network = Board::GetInstance().GetNetwork();
    mqtt_ = network->CreateMqtt(0);
    mqtt_->SetKeepAlive(keepalive);

    mqtt_->OnConnected([this]()
                       { OnConnect(0); });

    mqtt_->OnDisconnected([this]()
                          { OnDisconnect(0); });

    mqtt_->OnMessage([this](const std::string &topic, const std::string &payload)
                     { OnMessage(topic, (char *)payload.c_str(), payload.size()); });

    LOGI("Connecting to host %s", host.c_str());
    if (!mqtt_->Connect(host, port, clientId, username, password))
    {
        LOGE("Failed to connect to endpoint");
        // SetError(Lang::Strings::SERVER_NOT_CONNECTED);
        return CODE_ERROR;
    }

    LOGI("Connected to endpoint");

    return CODE_OK;
}

int MyMqtt::Connect(int timeout)
{
    // return

    for (auto &action : actionCallbacks)
    {
        if (!action.GetState())
        {
            Subscribe(action.GetMsgIdPtr(), action.getTopic());
            // return;
        }
    }
    return 0;
}

int MyMqtt::Subscribe(int *msgId, string topic)
{
    return mqtt_ && mqtt_->Subscribe(topic, 0) ? CODE_OK : CODE_ERROR;
}

int MyMqtt::Unsubscribe(int *msgId, string topic)
{
    return mqtt_ && mqtt_->Unsubscribe(topic) ? CODE_OK : CODE_ERROR;
}

int MyMqtt::Publish(string topic, const char *payload, int payloadLen)
{
    return mqtt_ && mqtt_->Publish(topic, string(payload, payloadLen)) ? CODE_OK : CODE_ERROR;
}

int MyMqtt::Publish(string topic, string payload)
{
    return mqtt_ && mqtt_->Publish(topic, payload) ? CODE_OK : CODE_ERROR;
}

bool MyMqtt::IsConnected()
{
    return mqtt_ && mqtt_->IsConnected();
}

void MyMqtt::OnConnect(int rc)
{
    // MqttWrap::OnConnect(rc);

    LOGI("OnConnect");
    // DeviceManager::GetInstance()->ledConf->Control(1);
    SubscribeList();
    // if (oldState == false)
    // 	CheckTrigger();
}

void MyMqtt::OnDisconnect(int rc)
{
    // MqttWrap::OnDisconnect(rc);

    LOGW("OnDisconnect");
    // bool oldState = isConnected;
    // DeviceManager::GetInstance()->ledConf->Control(0);
    for (auto &action : actionCallbacks)
    {
        action.SetState(false);
    }
    // if (oldState == true)
    // 	CheckTrigger();
}

void MyMqtt::OnPublish(int msgId)
{
    // MqttWrap::OnPublish(msgId);
}

void MyMqtt::OnSubscribe(int msgId)
{
    // MqttWrap::OnSubscribe(msgId);

    for (auto &action : actionCallbacks)
    {
        if (action.GetMsgId() == msgId)
        {
            action.SetState(true);
        }
    }
    SubscribeList();
}

void MyMqtt::OnSubscribeDone()
{
}

void MyMqtt::OnUnsubscribe(int msgId)
{
    // MqttWrap::OnUnsubscribe(msgId);

    for (auto &action : actionCallbacks)
    {
        if (action.GetMsgId() == msgId)
        {
            action.SetState(false);
        }
    }
}

void MyMqtt::OnMessage(string topic, char *payload, int payloadLen)
{
    // MqttWrap::OnMessage(topic, payload, payloadLen);

    // DeviceManager::GetInstance()->ledConf->Control(0);
    string msg = string(payload, payloadLen);
    LOGD("OnMessage topic: %s", topic.c_str());
    ActionCallback *actionCallback;
    if (FindActionCallbackFuncFromTopic(topic, &actionCallback) == CODE_OK)
    {
        if (actionCallback->getType() == 1)
        {
            string payloadStr = string(payload, payloadLen);
            actionCallback->actionCallbackFuncType1(topic, payloadStr);
        }
        else if (actionCallback->getType() == 2)
        {
            actionCallback->actionCallbackFuncType2(topic, payload, payloadLen);
        }
        else if (actionCallback->getType() == 3)
        {
            string payloadStr = string(payload, payloadLen);
            actionCallback->actionCallbackFuncType3(topic, payloadStr);
        }
        else if (actionCallback->getType() == 4)
        {
            actionCallback->actionCallbackFuncType4(topic, payload, payloadLen);
        }
    }
    // DeviceManager::GetInstance()->ledConf->Control(1);
}

void MyMqtt::AddActionCallback(ActionCallbackFuncType1 actionCallbackFuncType1, string topic)
{
    ActionCallback actionCallback(actionCallbackFuncType1, topic);
    actionCallbacks.push_back(actionCallback);
    Subscribe(actionCallback.GetMsgIdPtr(), topic);
}

void MyMqtt::AddActionCallback(ActionCallbackFuncType2 actionCallbackFuncType2, string topic)
{
    ActionCallback actionCallback(actionCallbackFuncType2, topic);
    actionCallbacks.push_back(actionCallback);
    Subscribe(actionCallback.GetMsgIdPtr(), topic);
}

void MyMqtt::AddActionCallback(ActionCallbackFuncType3 actionCallbackFuncType3, string topic)
{
    ActionCallback actionCallback(actionCallbackFuncType3, topic);
    actionCallbacks.push_back(actionCallback);
    Subscribe(actionCallback.GetMsgIdPtr(), topic);
}

void MyMqtt::AddActionCallback(ActionCallbackFuncType4 actionCallbackFuncType4, string topic)
{
    ActionCallback actionCallback(actionCallbackFuncType4, topic);
    actionCallbacks.push_back(actionCallback);
    Subscribe(actionCallback.GetMsgIdPtr(), topic);
}

int checkMqttTopic(string retrieveTopic, string registerTopic)
{
    vector<string> retrieveList = Util::splitString(retrieveTopic, '/');
    vector<string> registerList = Util::splitString(registerTopic, '/');
    for (size_t i = 0; i < retrieveList.size(); i++)
    {
        // fix MQTT check topic bug https://github.com/thinpv/smh_HC/commit/cf90cea3695d99ce10d8c8e6e048031e77923236
        if (registerList.size() <= i)
            return CODE_ERROR;
        if (registerList.at(i) == "#")
            return CODE_OK; // OK
        if (registerList.at(i) == "+")
            continue;
        if (registerList.at(i) != retrieveList.at(i))
            return CODE_ERROR;
    }
    if (registerList.size() == retrieveList.size())
        return CODE_OK; // OK
    return CODE_ERROR;
}

int MyMqtt::FindActionCallbackFuncFromTopic(string topic, ActionCallback **actionCallback)
{
    for (auto &action : actionCallbacks)
    {
        if (checkMqttTopic(topic, action.getTopic()) == CODE_OK)
        {
            *actionCallback = &action;
            return CODE_OK;
        }
    }
    return CODE_ERROR;
}

void MyMqtt::SubscribeList()
{
    for (auto &action : actionCallbacks)
    {
        if (!action.GetState())
        {
            Subscribe(action.GetMsgIdPtr(), action.getTopic());
            return;
        }
    }
    OnSubscribeDone();
}

bool MyMqtt::CheckData(Json::Value &dataValue, bool &rs)
{
    LOGD("CheckData data: %s", dataValue.toString().c_str());
    if (dataValue.isObject() &&
        dataValue.isMember("value") &&
        dataValue.isMember("op") && dataValue["op"].isString())
    {
        string op = dataValue["op"].asString();
        if (dataValue["value"].isInt())
        {
            int value = dataValue["value"].asInt();
            rs = Util::CompareNumber(op, IsConnected(), value);
            return true;
        }
        else if (dataValue["value"].isArray())
        {
            Json::Value listValue = dataValue["value"];
            if (listValue.size() == 2 && listValue[0].isInt() && listValue[1].isInt())
            {
                int value1 = listValue[0].asInt();
                int value2 = listValue[1].asInt();
                rs = Util::CompareNumber(op, IsConnected(), value1, value2);
                return true;
            }
        }
    }
    return false;
}