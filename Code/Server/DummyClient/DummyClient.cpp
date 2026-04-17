#include "pch.h"
#include "StressTest.h"

int main()
{
	StressTestConfig config;

	std::cout << "========================================" << std::endl;
	std::cout << "   Majestro Dummy Client - Stress Test  " << std::endl;
	std::cout << "========================================" << std::endl;

	while (true) {
		std::cout << std::endl;
		std::cout << "  Current Settings:" << std::endl;
		std::cout << "  [1] Server IP:Port   = " << config.serverIp << ":" << config.tcpPort << std::endl;
		std::cout << "  [2] Client Count     = " << config.totalClients << std::endl;
		std::cout << "  [3] Ramp-up/sec      = " << config.rampUpPerSecond << std::endl;
		std::cout << "  [4] Move Send Rate   = " << (int)(1.0f / config.moveSendInterval) << " Hz" << std::endl;
		std::cout << "  [5] Action Interval  = " << config.actionInterval << " sec" << std::endl;
		std::cout << "  [6] Test Duration    = "
			<< (config.testDuration > 0 ? std::to_string((int)config.testDuration) + " sec" : std::string("Infinite"))
			<< std::endl;
		std::cout << std::endl;
		std::cout << "  [S] Start Test" << std::endl;
		std::cout << "  [Q] Quit" << std::endl;
		std::cout << std::endl;
		std::cout << "  > ";

		std::string input;
		std::getline(std::cin, input);

		if (input.empty()) continue;

		char cmd = (char)toupper(input[0]);

		switch (cmd) {
		case '1':
		{
			std::cout << "  Server IP (current: " << config.serverIp << "): ";
			std::string ip;
			std::getline(std::cin, ip);
			if (!ip.empty()) config.serverIp = ip;

			std::cout << "  TCP Port (current: " << config.tcpPort << "): ";
			std::string portStr;
			std::getline(std::cin, portStr);
			if (!portStr.empty()) config.tcpPort = std::stoi(portStr);
			config.udpPort = config.tcpPort + 1;
			break;
		}
		case '2':
		{
			std::cout << "  Client Count: ";
			std::string val;
			std::getline(std::cin, val);
			if (!val.empty()) config.totalClients = std::stoi(val);
			break;
		}
		case '3':
		{
			std::cout << "  Ramp-up per second: ";
			std::string val;
			std::getline(std::cin, val);
			if (!val.empty()) config.rampUpPerSecond = std::stoi(val);
			break;
		}
		case '4':
		{
			std::cout << "  Move Send Rate (Hz): ";
			std::string val;
			std::getline(std::cin, val);
			if (!val.empty()) {
				float hz = std::stof(val);
				if (hz > 0) config.moveSendInterval = 1.0f / hz;
			}
			break;
		}
		case '5':
		{
			std::cout << "  Action Interval (sec): ";
			std::string val;
			std::getline(std::cin, val);
			if (!val.empty()) config.actionInterval = std::stof(val);
			break;
		}
		case '6':
		{
			std::cout << "  Test Duration (sec, 0=infinite): ";
			std::string val;
			std::getline(std::cin, val);
			if (!val.empty()) config.testDuration = std::stof(val);
			break;
		}
		case 'S':
		{
			StressTestManager manager;
			if (manager.Initialize(config)) {
				manager.Run();
				manager.Shutdown();
			}
			else {
				std::cout << "[ERROR] 스트레스 테스트 초기화 실패" << std::endl;
			}
			break;
		}
		case 'Q':
			return 0;
		}
	}

	return 0;
}
