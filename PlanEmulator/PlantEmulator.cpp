#pragma once

#include <iostream>
#include <string>
#include <sstream>
#include <chrono> 
#include <iomanip>
#include "PlantEmulator.h"

using namespace std;

PlantEmulator::PlantEmulator(ControlData& cD, const char* fpath) : cDataCat(cD) {
    if (cDataCat.pBuf == nullptr) {
        cout << "Allocating plant buffer";
        cDataCat.pBuf = new vector<unsigned char>;
    }
    this->filepath = fpath;
    LOG(this->filepath);
    this->loadData4();
}

PlantEmulator::~PlantEmulator() {
    if (hDLL != nullptr) {
        FreeLibrary(hDLL);
    }
    if (!cDataCat.pBuf->empty()) {
        delete cDataCat.pBuf;
        cout << "Clearing buf" << endl;
    }
    for (auto& channelEntry : this->Data4) {
        for (auto& pointEntry : *channelEntry.second) {
            if (pointEntry.second) {
                LOG("Deleting data point");
                delete pointEntry.second;  // Delete lists
            }
        }
        if (channelEntry.second) {
            LOG("Deleting data channel");
            delete channelEntry.second;  // Delete inner maps
        }
    }
}


void PlantEmulator::LoadDLL() {

    this->hDLL = LoadLibraryA("C:\\Users\\S11M\\source\\repos\\powerplant\\powerplant\\IAS0410PlantEmulator.dll");
    if (hDLL == nullptr) {
        cout << "DLL not found, error " << GetLastError() << endl;
        throw runtime_error("Failed to load DLL");
    }
    cout << "DLL loaded successfully" << endl;
    this->DLLAttached = true;
}

void PlantEmulator::InitializePlant(const  string& filePath, int plantNumber)
{
	if (hDLL == nullptr) {
		throw runtime_error("DLL not loaded");
	}

	FARPROC setEmulatorProc = GetProcAddress(hDLL, "SetIAS0410PlantEmulator");

	if (setEmulatorProc == NULL) {
		cout << "SetIAS0410PlantEmulator() not found" << GetLastError() << endl;
		throw runtime_error("Function not found in DLL");
	}

	void (*SetPlant)(string, int) = reinterpret_cast<void (*)(string, int)>(setEmulatorProc);

	try {
		SetPlant(filePath, plantNumber);
	}
	catch (exception& e) {
		cerr << e.what() << endl;
		throw runtime_error("Failed to initialize plant");
	}
}

void PlantEmulator::RunPlant()
{
	if (hDLL == nullptr) {
		throw runtime_error("DLL not loaded");
	}

	FARPROC runEumlatorProc = GetProcAddress(hDLL, "RunIAS0410PlantEmulator");

	if (runEumlatorProc == NULL) {
		cerr << "RunIAS0410PlantEmulator() not found" << GetLastError() << endl;
		throw runtime_error("Function not found in DLL");
	}

	void (*RunPlantFunc)(ControlData*) = reinterpret_cast<void (*)(ControlData*)>(runEumlatorProc);

    if (cDataCat.pBuf == nullptr) {
        cout << "Allocating plant buffer THIS SHOULD NOT HAPPEN";
        cDataCat.pBuf = new vector<unsigned char>;
    }

	try {
		cout << "Running plant" << endl;
		RunPlantFunc(&cDataCat);
	}
	catch (exception& e) {
		cerr << "Exception: " << e.what() << endl;
		throw runtime_error("Failed to run plant");
	}
}


bool PlantEmulator::isDLLAttached()
{
    return this->DLLAttached;
}

const datastruct4& PlantEmulator::getData() const
{
    return this->Data4;
}


//void PlantEmulator::addData4(const string& channelName, const string& pointName, const variant<int, double>& value) {
//    // Check if the channel already exists
//    //auto channelIt = Data4.find(channelName);
//    //if (channelIt == Data4.end()) {
//    //    // Channel doesn't exist, create a new inner map
//    //    auto innerMap = new map<string, list<pair<variant<int, double>, chrono::system_clock::time_point>>*>;
//    //    Data4[channelName] = innerMap;
//    //}
//    //// Access the inner map for the channel
//    //auto& innerMap = *Data4[channelName];
//    //// Check if the point already exists
//    //auto pointIt = innerMap.find(pointName);
//    //if (pointIt == innerMap.end()) {
//    //    // Point doesn't exist, create a new list
//    //    auto dataList = new list<pair<variant<int, double>, chrono::system_clock::time_point>>;
//    //    innerMap[pointName] = dataList;
//    //}
//    //// Access the list for the point
//    //auto& dataList = *innerMap[pointName];
//    //// Add the data to the list
//    //dataList.push_back({ value, chrono::system_clock::now() });
//}
//
void PlantEmulator::printData4()
{   
    DataManipulation::PrintData4(Data4);
    return;

    //for (const auto& channelEntry : Data4) {
    //    cout << "Ch:" << channelEntry.first << endl;
    //    for (const auto& pointEntry : *channelEntry.second) {
    //        for (const auto& dataEntry : *pointEntry.second) {
    //            // Print the value based on its type
    //            if (dataEntry.first.index() == 0) {
    //                cout << pointEntry.first << " = " << get<int>(dataEntry.first) << "\t";
    //            }
    //            else if (dataEntry.first.index() == 1) {
    //                cout << pointEntry.first << " = " << get<double>(dataEntry.first) << "\t";
    //            }
    //            else {
    //                cout << endl;
    //            }
    //            // Print the timestamp
    //            time_t now_t = chrono::system_clock::to_time_t(dataEntry.second);
    //            struct tm now_tm;
    //            localtime_s(&now_tm, &now_t);
    //            cout << put_time(&now_tm, "%d-%m-%Y %H:%M:%S ") << endl;
    //        }
    //    }
    //    cout << endl;
    //}
}

void PlantEmulator::loadData4()
{
    DataManipulation::ReadData4FromFile(this->filepath, &this->Data4);
}


void PlantEmulator::LaunchConsumerThread(thread &t1) {
    t1 = thread(&PlantEmulator::ConsumerThreadFun, this, &cDataCat);
}
// CREATE_ALWAYS
// OPEN_EXISTING
void PlantEmulator::ConsumerThreadFun(ControlData *cData)
{   
    HANDLE hFile;
    hFile = CreateFileA(this->filepath, GENERIC_READ | GENERIC_WRITE, 0,
        NULL, OPEN_ALWAYS, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        cout << "Some error " << GetLastError() << endl;
    }
    while (true) {
        unique_lock<mutex> lock(cData->mx);

        if (cData->state == 's') {
            break;
        }

        cData->cv.wait(lock, [&]() { return !cData->pBuf->empty(); });

        // Check if there is data in pBuf
        if (!cData->pBuf->empty()) {
            DataManipulation::ParseData4(cData->pBuf, &this->Data4);
            DataManipulation::WriteData4ToFile(hFile, cData->pBuf);


           // vector<unsigned char>& receivedData = *(cData->pBuf);
           // // First 4 bytes
           // int packageLength = *reinterpret_cast<int*>(&receivedData[0]);
           // 
           // // Bytes 4-7
           // int numChannels = *reinterpret_cast<int*>(&receivedData[4]);
           // stringstream sout;
           // chrono::system_clock::time_point t = chrono::system_clock::now();
           // time_t now_t = chrono::system_clock::to_time_t(t);
           // struct tm now_tm;
           // localtime_s(&now_tm, &now_t);
           // sout << put_time(&now_tm, "%d-%m-%Y %H:%M:%S ");
           // sout << "Received: " << packageLength << " bytes" << endl;
           // size_t currentIndex = 8;
           // // Iterate through channels
           // for (int i = 0; i < numChannels; ++i) {
           //     // Parse the number of points in the channel
           //     int numPoints = *reinterpret_cast<int*>(&receivedData[currentIndex]);
           //     currentIndex += 4;
           //     // Parse the channel name
           //     string channelName(reinterpret_cast<char*>(&receivedData[currentIndex]));
           //     currentIndex += channelName.size() + 1; // Move to the next byte after the null terminator
           //     sout << endl << "Ch" << i + 1 << " " << channelName << ":" << endl;
           //     for (int j = 0; j < numPoints; ++j) {
           //         // Parse the point name
           //         string pointName(reinterpret_cast<char*>(&receivedData[currentIndex]));
           //         currentIndex += size_t(pointName.size() + 1); // Move to the next byte after the null terminator
           //         string lower = pointName;
           //         transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
           //         bool useInt = lower.find("level") != string::npos || lower.find("concentration") != string::npos;
           //         double measuredValue;
           //         // Parse the measured value based on its type
           //         if (useInt) {
           //             // Integer value
           //             measuredValue = *reinterpret_cast<int*>(&receivedData[currentIndex]);
           //             currentIndex += sizeof(int);
           //         }
           //         else {
           //             // Double value
           //             measuredValue = *reinterpret_cast<double*>(&receivedData[currentIndex]);
           //             currentIndex += sizeof(double);
           //         }
           //         DataManipulation::faddData4(channelName, pointName, measuredValue, &this->Data4);
           //         // Print the parsed information (replace this with your actual processing logic)
           //         sout << pointName << " = " << measuredValue << endl;
           //     }
           // }
           // sout << "Package len: " << packageLength << " Index: " << currentIndex << endl << endl;
           // cout << sout.str();
           //// Clear the buffer after processing


            cData->pBuf->clear();
            //this_thread::sleep_for(chrono::seconds(1));
        }
        cData->cv.notify_one();
    }
    CloseHandle(hFile);
    LOG("Exiting consumer")
}

void PlantEmulator::DisconnectDLL()
{
    // Disconnect DLL
    if (this->DLLAttached) {
        FreeLibrary(hDLL);
    }

    this->DLLAttached = false;
}

