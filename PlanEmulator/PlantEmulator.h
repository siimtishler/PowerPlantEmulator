#pragma once
#include "common.h"
#include <windows.h>
#include <thread>
#include "DataManipulation.h"

class PlantEmulator
{
private:

	ControlData& cDataCat;

	HMODULE hDLL = nullptr;

	datastruct4 Data4;

	bool DLLAttached = false;

	const char* filepath;

public:

	PlantEmulator(ControlData&, const char*);
	~PlantEmulator();

	void LoadDLL();
	void InitializePlant(const std::string& , int plantNumber);
	void RunPlant();
	void LaunchConsumerThread(std::thread &);
	void ConsumerThreadFun(ControlData *); // Consumer thread function
	void DisconnectDLL();

	bool isDLLAttached();

	const datastruct4& getData() const;
	void printData4();

	void loadData4();

	friend class DataManipulation;
};

