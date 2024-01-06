#pragma once
#include "common.h"
#include <windows.h>
#include <thread>
#include "DataManipulation.h"

class PlantEmulator
{
public:
	/*typedef std::map<std::string, std::map<std::string, std::list<std::pair<std::variant<int, double>, std::chrono::system_clock::time_point> >* >* > datastruct4;*/

private:

	ControlData& cDataCat;
	HMODULE hDLL = nullptr;
	bool DLLAttached = false;

	/*std::map<std::string, std::map<std::string, std::list<std::pair<std::variant<int, double>, std::chrono::system_clock::time_point> >* >* > Data4;*/
	 datastruct4 Data4;

public:
	

	PlantEmulator(ControlData&);
	~PlantEmulator();

	void LoadDLL();
	void InitializePlant(const std::string& , int plantNumber);
	void RunPlant();
	void LaunchConsumerThread(std::thread &);
	void ConsumerThreadFun(ControlData *); // Consumer thread function

	void DisconnectDLL();

	bool isDLLAttached();

	const datastruct4& getData() const;

	//void addData4(const std::string& channelName, const std::string& pointName, const std::variant<int, double>& value);

	//void printData4();

	friend class DataManipulation;
};

