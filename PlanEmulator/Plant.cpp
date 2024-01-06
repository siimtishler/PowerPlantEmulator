#include <iomanip>
#include <stdio.h>
#include <windows.h>
#include <mutex>
#include <vector>
#include <condition_variable>
#include <future>
#include <map>
#include <list>
#include <algorithm>

#include <thread>

#include <sstream>
#include <fstream>

#include <conio.h>
#include <ctype.h>

#include <random>

#include "PlantEmulator.h"
#include "Controller.h"
#include "Windows.h"
#include "common.h"

#pragma warning(disable: 4996)

using namespace std;

HMODULE hDLL = nullptr;

ControlData* cData = nullptr;

void LoadDLL() {

	hDLL = LoadLibraryA("C:\\Users\\S11M\\source\\repos\\powerplant\\powerplant\\IAS0410PlantEmulator.dll");
	if (hDLL == nullptr) {
		cout << "DLL not found, error " << GetLastError() << endl;
		throw runtime_error("Failed to load DLL");
	}
	cout << "DLL loaded successfully" << endl;
}

void InitializePlant(const  string& filePath, int plantNumber)
{
	if (hDLL == nullptr) {
		throw runtime_error("DLL not loaded");
	}

	FARPROC ptr2 = GetProcAddress(hDLL, "SetIAS0410PlantEmulator");

	if (ptr2 == NULL) {
		cout << "SetIAS0410PlantEmulator() not found" << GetLastError() << endl;
		throw runtime_error("Function not found in DLL");
	}

	void (*SetPlant)(string, int) = reinterpret_cast<void (*)(string, int)>(ptr2);

	try {
		SetPlant(filePath, plantNumber);
	}
	catch (exception& e) {
		cerr << e.what() << endl;
		throw runtime_error("Failed to initialize plant");
	}
}

void RunPlant()
{
	if (hDLL == nullptr) {
		throw runtime_error("DLL not loaded");
	}

	FARPROC ptr1 = GetProcAddress(hDLL, "RunIAS0410PlantEmulator");

	if (ptr1 == NULL) {
		cerr << "RunIAS0410PlantEmulator() not found" << GetLastError() << endl;
		throw runtime_error("Function not found in DLL");
	}

	void (*RunPlantFunc)(ControlData*) = reinterpret_cast<void (*)(ControlData*)>(ptr1);

	if (cData->pBuf == nullptr) {
		cout << "Allocating plant buffer THIS SHOULD NOT HAPPEN";
		cData->pBuf = new std::vector<unsigned char>;
	}

	try {
		cout << "Running plant" << endl;
		RunPlantFunc(cData);
	}
	catch (exception& e) {
		cerr << "Exception: " << e.what() << endl;
		throw runtime_error("Failed to run plant");
	}
}


class Controller2
{
public:
	atomic<char>& state;
	Controller2(atomic<char>& b) : state(b) { }
	void operator() ()
	{
		while (true) {
			char buf[100];
			cin >> buf;
			if (strstr(buf, "stop")) {
				state = 's';
				LOG("STOPPING");
			}
			else if (strstr(buf, "start")) {
				state = 'r';
				LOG("STARTING");
			}
			else if (strstr(buf, "exit")) {
				state = 's';
				LOG("EXITING");
				break;
			}
			else if (strstr(buf, "break")) {
				state = 'b';
				LOG("BREAKING");
				break;
			}
			else {
				cout << "TYPED: " << buf << endl;
			}
		}
	}
};

class Producer {
public: 
	ControlData* cData;
	  int buf_size;
	  Producer(ControlData* cD, int s) : cData(cD), buf_size(s) { }
	  void operator() () {
		  default_random_engine generator;
		  uniform_int_distribution<int> delay(0, 1000);
		  uniform_int_distribution<int> random_number(0, 100);
		  cout << "Starting producer on ";
		  LOG(this_thread::get_id());
		  while (true) {

			  unique_lock<mutex> lock(cData->mx); // locks the mutex

			  if (cData->state == 's') {
				  break;
			  }
			  this_thread::sleep_for(chrono::milliseconds(delay(generator)));
			  if (cData->state == 'b') {
				  cout << "Currently not producing" << endl;
				  continue;
			  }

			  cData->pBuf->resize(buf_size);
			  generate(cData->pBuf->begin(), cData->pBuf->end(), [&]() { return random_number(generator); });
			  cData->cv.notify_one(); // releases the comsumer
			  cData->cv.wait(lock); // blocks the producer
		  }
		  cData->cv.notify_all();
		  cData->pProm->set_value();
		  LOG("Producer exiting...");
	  }
};

class Consumer {
public: 
	ControlData* cData;
	Consumer(ControlData *cD) : cData(cD){ }

	void operator() () {

		cout << "Starting consumer on ";
		LOG(this_thread::get_id());
		
		while (true) {
			unique_lock<mutex> lock(cData->mx); // locks the mutex

			if (cData->state == 's') {
				cout << "Stopped consumer";
				break;
			}

			cData->cv.wait(lock, [&]() { return !cData->pBuf->empty(); });


		if (!cData->pBuf->empty()) {
		    // Parse the received data
		    vector<unsigned char>& receivedData = *(cData->pBuf);
		
		    // First 4 bytes
		    int packageLength = *reinterpret_cast<int*>(&receivedData[0]);
		
		    // Bytes 4-7
		    int numChannels = *reinterpret_cast<int*>(&receivedData[4]);
		
		
		    stringstream sout;
		
		    chrono::system_clock::time_point t = chrono::system_clock::now();
		    time_t now_t = chrono::system_clock::to_time_t(t);
		    struct tm now_tm;
		    localtime_s(&now_tm, &now_t);
		    sout << put_time(&now_tm, "%d-%m-%Y %H:%M:%S ");
		    sout << "Received: " << packageLength << " bytes" << endl;
		
		
		    size_t currentIndex = 8;
		
		    // Iterate through channels
		    for (int i = 0; i < numChannels; ++i) {
		        // Parse the number of points in the channel
		        int numPoints = *reinterpret_cast<int*>(&receivedData[currentIndex]);
		        currentIndex += 4;
		
		        // Parse the channel name
		        string channelName(reinterpret_cast<char*>(&receivedData[currentIndex]));
		        currentIndex += channelName.size() + 1; // Move to the next byte after the null terminator
		
		        sout << endl << "Ch" << i + 1 << " " << channelName << ":" << endl;
		
		        for (int j = 0; j < numPoints; ++j) {
		            // Parse the point name
		
		            string pointName(reinterpret_cast<char*>(&receivedData[currentIndex]));
		            currentIndex += size_t(pointName.size() + 1); // Move to the next byte after the null terminator
		
		            string lower = pointName;
		
		            std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
		
		            bool useInt = lower.find("level") != string::npos || lower.find("concentration") != string::npos;
		
		            double measuredValue;
		
		            // Parse the measured value based on its type
		            if (useInt) {
		                // Integer value
		                measuredValue = *reinterpret_cast<int*>(&receivedData[currentIndex]);
		                currentIndex += sizeof(int);
		            }
		            else {
		                // Double value
		                measuredValue = *reinterpret_cast<double*>(&receivedData[currentIndex]);
		                currentIndex += sizeof(double);
		            }
		
		            // Print the parsed information (replace this with your actual processing logic)
		            sout << pointName << " = " << measuredValue << endl;
		        }
		    }
		    sout << "Package len: " << packageLength << " Index: " << currentIndex << endl << endl;
		    cout << sout.str();
		   // Clear the buffer after processing
		    cData->pBuf->clear();
		
		    this_thread::sleep_for(chrono::seconds(1));
		}
			//for_each(cData->pBuf->begin(), cData->pBuf->end(), [](int i) { cout << i << ' '; });
			cData->pBuf->resize(0);
			cout << endl;
			cData->cv.notify_one(); // releases the producer
		}
		LOG("Consumer exiting...");
	}
};


void deletecData(ControlData*& cD) {
	if (cD) {
		LOG("DELETING");
		delete cD->pBuf;
		delete cD->pProm;
		delete cD;
		cD = nullptr;
	}
}

void initcData(ControlData*& cD, future<void> &fut) {
	if (cD == nullptr) {
		LOG("INITING");
		cD = new ControlData;
		cD->pBuf = new std::vector<unsigned char>;
		cD->pProm = new std::promise<void>;
		//fut = cD->pProm->get_future();
	}
}

// 02 - 01 - 2024 18:33 : 52 Received : 273 bytes
// 
// Ch1 Heater :
// Air inlet flow = 6.29457
// Inside temperature = 520.98
// Gas inlet flow = 4.24541
// Gas outlet flow = 3.09548
// 
// Ch2 Solution tank :
// Solution conductivity = 4.72862
// Solution concentration = 85
// Solution pH = 8.28552
// Level = 80
// 
// Ch3 Scrubber :
// Inlet flow = 9.50263
// Outlet flow = 6.28668
// Package len : 273 Index : 273

map<string, map<string, list<pair<variant<int, double>, chrono::system_clock::time_point> >* >* > Data4;

void addData(const std::string& channelName, const std::string& pointName, const std::variant<int, double>& value) {
	// Check if the channel already exists
	auto channelIt = Data4.find(channelName);

	if (channelIt == Data4.end()) {
		// Channel doesn't exist, create a new inner map
		auto innerMap = new std::map<std::string, std::list<std::pair<std::variant<int, double>, std::chrono::system_clock::time_point>>*>;
		Data4[channelName] = innerMap;
	}

	// Access the inner map for the channel
	auto& innerMap = *Data4[channelName];

	// Check if the point already exists
	auto pointIt = innerMap.find(pointName);

	if (pointIt == innerMap.end()) {
		// Point doesn't exist, create a new list
		auto dataList = new std::list<std::pair<std::variant<int, double>, std::chrono::system_clock::time_point>>;
		innerMap[pointName] = dataList;
	}

	// Access the list for the point
	auto& dataList = *innerMap[pointName];

	// Add the data to the list
	dataList.push_back({ value, std::chrono::system_clock::now() });
}

void printData()  {
	for (const auto& channelEntry : Data4) {
		std::cout << "Ch:" << channelEntry.first << std::endl;

		for (const auto& pointEntry : *channelEntry.second) {

			for (const auto& dataEntry : *pointEntry.second) {
				// Print the value based on its type
				if (dataEntry.first.index() == 0) {
					std::cout << pointEntry.first << " = " << std::get<int>(dataEntry.first) << "\t\t";
					//std::cout << "    Value (int): " << std::get<int>(dataEntry.first) << " ";
				}
				else if (dataEntry.first.index() == 1) {
					std::cout << pointEntry.first << " = " << std::get<double>(dataEntry.first) << "\t\t";
					//std::cout << "    Value (double): " << std::get<double>(dataEntry.first)<< " ";
				}
				else {
					cout << endl;
				}

				// Print the timestamp
				time_t now_t = chrono::system_clock::to_time_t(dataEntry.second);
				struct tm now_tm;
				localtime_s(&now_tm, &now_t);
				cout << put_time(&now_tm, "%d-%m-%Y %H:%M:%S ") << endl;
			}
		}
		cout << endl;
	}
}

int main(int argc, char **argv)
{
	// Channel name -> Points map ptr -> List ptr ([Measurment int/double], timestamp)

	chrono::system_clock::time_point t = chrono::system_clock::now();

	vector<string>channels = { "Heater","Solution Tank","Scrubber" };
	vector<string>points1 = { "Air inlet flow", "Inside temperature","Gas inlet flow","Gas outlet flow" };
	vector<string>points2 = { "Solution conductivity" ,"Solution concentration", "Solution pH", "Level"};
	vector<string>points3 = {"Inlet flow", "Outlet flow", "Level"};

	default_random_engine generator;
	uniform_real_distribution<double> doubles(0, 10);

	int i = 1;
	for (auto ch : channels) {
		cout << ch << ":" << endl;


		if (i == 1) {
			for (auto p1 : points1) {
				int val = doubles(generator);
				cout << p1 << " = " << val << endl;
				addData(ch, p1, val);
			}
		}
		if (i == 2) {
			for (auto p2 : points2) {
				double val = doubles(generator);
				cout << p2 << " = " << val << endl;
				addData(ch, p2, val);
			}
		}
		if (i == 3) {
			for (auto p3 : points3) {
				double val = doubles(generator);
				cout << p3 << " = " << val << endl;
				addData(ch, p3, val);
			}
			i = 1;
		}
		i++;
		cout << endl;
	}
	//printData();

	/*if (argc) {
		for (int i = 0; i < argc; i++) {
			cout << argv[i] << endl;
		}
	}*/
	//std::vector<unsigned char> data = { 42, 55, 78, 91 };
	//HANDLE hFile = CreateFileA("Data44.bin", GENERIC_READ | GENERIC_WRITE, 0,
	//	NULL, CREATE_ALWAYS, 0, NULL);
	//if (hFile == INVALID_HANDLE_VALUE) {
	//	cout << "Some error " << GetLastError() << endl;
	//}

	//DataManipulation::WriteData4ToFile(hFile, &data);
	//DataManipulation::ReadData4FromFile(hFile);
	//CloseHandle(hFile);

	//return 1;

	//unsigned long nBytesToWrite = 1024, nWrittenBytes = 0;
	//unsigned char* pBuffer = new unsigned char[nBytesToWrite];

	//int Result = WriteFile(hFile, pBuffer, nBytesToWrite, &nWrittenBytes, NULL);
	//if (!Result)
	//	cout << "Data not written, error " << GetLastError() << endl;
	//else if (nBytesToWrite != nWrittenBytes)
	//	cout << "Only " << nWrittenBytes << " bytes instead of " << nBytesToWrite
	//	<< " was written" << endl;
	//else
	//	cout << nWrittenBytes << " bytes was written" << endl;



//
	thread prod;
	thread cons;

	future<void> fut;

	/*initcData(cData, fut);

	atomic<char>& state = cData->state;

	while (true) {
		char buf[100];
		cin >> buf;

		if (strstr(buf, "connect")) {
			try {
				LoadDLL();
				InitializePlant("C:\\Users\\S11M\\Desktop\\IAS0410Plants.txt", 6);
			}
			catch (const runtime_error& e) {
				LOG(e.what());
				cerr << "Couldn't Load and Init plant" << endl;
			}
		}

		if (strstr(buf, "stop")) {
			state = 's';
			LOG("STOPPING");

			cData->cv.notify_all();
			fut = cData->pProm->get_future();
			fut.wait();

			cons.join();
			deletecData(cData);
		}

		if (strstr(buf, "start")) {
			initcData(cData, fut);
			state = 'r';
			RunPlant();
			cons = thread{ Consumer(cData) };
			LOG("STARTING");
		}
		else if (strstr(buf, "exit")) {
			state = 's';
			cons.join();
			LOG("EXITING");
			return 1;
		}
		else if (strstr(buf, "break")) {
			state = 'b';
			LOG("BREAKING");
		}
		else if (strstr(buf, "resume")) {
			state = 'r';
			LOG("RESUMING");
		}
		else {
			cout << "TYPED: " << buf << endl;
		}
	}

	cons.join();
	LOG("EXITING PROG");


	return 1;*/


	const char* filepath = "C:\\Users\\S11M\\Desktop\\IAS0410Plants.txt";
	int plantNum = 6;

	ControlData cData2;
	
	cData2.pBuf = new vector<unsigned char>;
	cData2.pProm = new promise<void>;

	Controller contr(cData2);


	try {
		contr.controllerThreadFun();
	}
	catch (exception& e) {
		cerr << "Error" << e.what() << endl;
		return -1;
	}

	//CloseHandle(hFile);

	return 1;


	//PlantEmulator plant(cData);
	//contr.launchControllerThread();

	//try {
	//	plant.LoadDLL();
	//	plant.InitializePlant("C:\\Users\\S11M\\Desktop\\IAS0410Plants.txt", 6);
	//	plant.RunPlant();
	//}
	//catch (exception& e) {
	//	cerr << "Error: " << e.what() << endl;
	//	return 1;
	//}
	//plant.LaunchConsumerThread();
	

	// Add later

	//UINT WINAPI codepage = GetConsoleOutputCP();
	//SetConsoleOutputCP(1252);
	//SetConsoleOutputCP(codepage);

}
//
//
//// THIS USES CONSUMER FROM SLIDES AND PRODUCER FROM DLL. USES BUF TO CONTROL
//// SHOULD BE WORKING VERSION, CAN START STOP ETC
//
////thread prod;
////thread cons;
////
////future<void> fut;
////
////ControlData cData2;
////
////initcData(cData, fut);
////
////atomic<char>& state = cData->state;
////
////while (true) {
////	char buf[100];
////	cin >> buf;
////
////	if (strstr(buf, "connect")) {
////		try {
////			LoadDLL();
////			InitializePlant("C:\\Users\\S11M\\Desktop\\IAS0410Plants.txt", 6);
////		}
////		catch (const runtime_error& e) {
////			LOG(e.what());
////			cerr << "Couldn't Load and Init plant" << endl;
////		}
////	}
////
////	if (strstr(buf, "stop")) {
////		state = 's';
////		LOG("STOPPING");
////
////		cData->cv.notify_all();
////		fut = cData->pProm->get_future();
////		fut.wait();
////
////		cons.join();
////		deletecData(cData);
////	}
////
////	if (strstr(buf, "start")) {
////		initcData(cData, fut);
////		state = 'r';
////		RunPlant();
////		cons = thread{ Consumer(cData) };
////		LOG("STARTING");
////	}
////	else if (strstr(buf, "exit")) {
////		state = 's';
////		cons.join();
////		LOG("EXITING");
////		return 1;
////	}
////	else if (strstr(buf, "break")) {
////		state = 'b';
////		LOG("BREAKING");
////	}
////	else if (strstr(buf, "resume")) {
////		state = 'r';
////		LOG("RESUMING");
////	}
////	else {
////		cout << "TYPED: " << buf << endl;
////	}
////}
////
////cons.join();
////LOG("EXITING PROG");
////
////
////return 1;
//


//
//int main(int argc, char* argv[])
//{
//    try {
//
//        ControlData cData;
//        HMODULE hDLL = LoadLibraryA("C:\\Users\\S11M\\source\\repos\\powerplant\\powerplant\\IAS0410PlantEmulator.dll");
//        if (!hDLL)
//        {
//            cout << "IAS0410PlantEmulator.dll not found, error " << GetLastError() << endl;
//            return 0;
//        }
//        cout << "IAS0410PlantEmulator.dll linked successfully!" << endl;
//
//        FARPROC initEmu = GetProcAddress(hDLL, "SetIAS0410PlantEmulator");
//        if (!initEmu)
//        {
//            FreeLibrary(hDLL);
//            cout << "Function SetIAS0410PlantEmulator not found, error " << GetLastError() << endl;
//            return 0;
//        }
//
//        void (*setPlant)(string, int) = reinterpret_cast<void(*)(string, int)>(initEmu);
//        try
//        {
//            setPlant("C:\\Users\\S11M\\Desktop\\IAS0410Plants.txt", 6);
//            cout << "SetIAS0410PlantEmulator completed successfully!";
//        }
//        catch (exception& e) {
//            cerr << "Plant 1 could not be initialized, " << e.what() << endl;
//        }
//    }
//    catch (exception& e)
//    {
//        cerr << "Error: " << e.what() << endl;
//    }
//    return 0;
//}