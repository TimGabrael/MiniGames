#include <iostream>
#include "CommonCollection.h"
#include <chrono>
#include "Network/NetServer.h"
#include "Network/NetCommon.h"
#include <thread>

static void UpdateLimiter(float time, float updateTime)
{
	float totalSleepTime = (time - updateTime) * 1000.0f;

	auto start = std::chrono::high_resolution_clock::now();
	while(totalSleepTime > 4.0f)
	{
		std::this_thread::sleep_for(std::chrono::duration<float>(1.0f / 1000.0f));
		const auto now = std::chrono::high_resolution_clock::now();
		totalSleepTime -= std::chrono::duration<float>(now - start).count() * 1000.0f;
		start = now;
	}
	while (totalSleepTime > 0.0f)
	{
		const auto now = std::chrono::high_resolution_clock::now();
		totalSleepTime -= std::chrono::duration<float>(now - start).count() * 1000.0f;
		start = now;
	}
}


int main()
{
	InitNetworking();

	ServerData* server = new ServerData(DEBUG_IP, DEBUG_PORT);

	
	std::cout << "waiting for clients\n";
	auto now = std::chrono::high_resolution_clock::now();
	while (true)
	{
		const auto later = std::chrono::high_resolution_clock::now();
		const float dt = std::chrono::duration<float>(later - now).count();
		now = later;
		server->Update(dt);
		const auto updateTime = std::chrono::high_resolution_clock::now();
		UpdateLimiter(server->updateInterval_ms / 1000.0f, std::chrono::duration<float>(updateTime - now).count());
	}
	
	system("pause");

	return 0;
}