#include "TimerSchedule.h"
#include <unistd.h>
#include <algorithm>
#include "Util.h"
#include "Log.h"
#include "ErrorCode.h"
#include "Led.h"
#include "app_task.h"

TimerSchedule *TimerSchedule::GetInstance()
{
	static TimerSchedule *timerSchedule = NULL;
	if (!timerSchedule)
	{
		timerSchedule = new TimerSchedule();
	}
	return timerSchedule;
}

Timer::Timer(int index, int time, TimerCallbackFunc timerCallbackFunc)
{
	this->index = index;
	this->time = time;
	this->timerCallbackFunc = timerCallbackFunc;
	this->rule = NULL;
}

Timer::Timer(int index, int time, Rule *rule)
{
	this->index = index;
	this->time = time;
	this->rule = rule;
}

int Timer::GetIndex()
{
	return index;
}

bool Timer::IsAtTime(int time)
{
	if (this->time == time)
		return true;
	return false;
}

TimerSchedule::TimerSchedule()
{
	index = 0;
}

TimerSchedule::~TimerSchedule()
{
}

void TimerSchedule::init()
{
	LOGI("Start Timer init");
	queue = xQueueCreate(10, sizeof(Timer *));
	LOGI("Free memory: %d bytes, internal: %d bytes", esp_get_free_heap_size(), esp_get_free_internal_heap_size());

	if (!app_new_task([](void *data)
										{
	LOGI("Start Timer Thread");
	TimerSchedule *timerSchedule = (TimerSchedule *)data;
	int currentTimer, oldTimer = 0;
	while (1)
	{
		currentTimer = Util::GetCurrentTimeInDay();
		if (currentTimer != oldTimer)
		{
			// LOGI("Free memory: %d bytes, internal: %d bytes", esp_get_free_heap_size(), esp_get_free_internal_heap_size());
			timerSchedule->mtx.lock();
			for (auto &timer : timerSchedule->timerList)
			{
				if (timer->IsAtTime(currentTimer))
				{
					xQueueSend(timerSchedule->queue, (void *)&timer, (TickType_t)0);
				}
			}
			timerSchedule->mtx.unlock();
			oldTimer = currentTimer;
		}
		else
			vTaskDelay(pdMS_TO_TICKS(500));
	}
	vTaskDelete(NULL); },					 // Task function
										"TimerThread", // Task name
										5120,					 // Stack size (words, not bytes)
										this,					 // Param
										5							 // Priority
										))
	{
		LOGE("Failed to create TimerThread task");
	}

	if (!app_new_task([](void *data)
										{
	LOGI("TimerDoThread Start");
	TimerSchedule *timerSchedule = (TimerSchedule *)data;
	Timer *timer = NULL;
	while (1)
	{
		if (xQueueReceive(timerSchedule->queue, &timer, (TickType_t)5))
		{
			if (timer)
			{
				if (timer->rule)
					timer->rule->Check();
				else
					timer->timerCallbackFunc();
			}
		}
		else
			vTaskDelay(pdMS_TO_TICKS(200));
	}
	vTaskDelete(NULL); },						 // Task function
										"TimerDoThread", // Task name
										10240,					 // Stack size (words, not bytes)
										this,						 // Param
										5								 // Priority
										))
	{
		LOGE("Failed to create TimerDoThread task");
	}
	LOGI("Free memory: %d bytes, internal: %d bytes", esp_get_free_heap_size(), esp_get_free_internal_heap_size());

	vTaskDelay(10);
}

int TimerSchedule::RegisterTimer(string timerStr, TimerCallbackFunc timerCallbackFunc)
{
	int time = Util::ConvertStrTimeToInt(timerStr);
	return RegisterTimer(time, timerCallbackFunc);
}

int TimerSchedule::RegisterTimer(int time, TimerCallbackFunc timerCallbackFunc)
{
	Timer *timer = new Timer(++index, time, timerCallbackFunc);
	if (timer)
	{
		mtx.lock();
		timerList.push_back(timer);
		mtx.unlock();
	}
	return index;
}

int TimerSchedule::RegisterTimer(string timerStr, Rule *rule)
{
	int time = Util::ConvertStrTimeToInt(timerStr);
	return RegisterTimer(time, rule);
}

int TimerSchedule::RegisterTimer(int time, Rule *rule)
{
	Timer *timer = new Timer(++index, time, rule);
	if (timer)
	{
		mtx.lock();
		timerList.push_back(timer);
		mtx.unlock();
	}
	return index;
}

int TimerSchedule::UnregisterTimer(int index)
{
	mtx.lock();
	for (auto &timer : timerList)
	{
		if (timer->GetIndex() == index)
		{
			timerList.erase(remove(timerList.begin(), timerList.end(), timer), timerList.end());
			delete timer;
		}
	}
	mtx.unlock();
	return CODE_OK;
}
