#pragma once
#include "common.h"
#include "PlantEmulator.h"


constexpr unsigned int hash(const char* s, int off = 0);

class Controller : public PlantEmulator {
private:
	ControlData &cData;

	std::thread consumerT;

	std::future<void> fut;

	static constexpr unsigned int hash(const char* s, int off = 0) {
		return !s[off] ? 5381 : (hash(s, off + 1) * 33) ^ s[off];
	}
	bool producerRunning();
	bool producerBreaked();

	void Connect();
	void Disconnect();
	void Start();
	void Stop();
	void Break();
	void Resume();
	void Print(const std::string&, const std::string&);
	void Limits(const std::string&, const std::string&);
	void Exit();

	void getCmdChAndPoint(std::string&, std::string&, std::string&, std::string&);

public:
	Controller(ControlData&, const char*);
	~Controller();

	void deletecData();
	void initcData();
	void controllerThreadFun();

	friend class DataManipulation;
};

