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
    DataManipulation::ReadData4FromFile(this->filepath, &this->Data4);
    //this->loadData4();
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

void PlantEmulator::DisconnectDLL()
{
    // Disconnect DLL
    if (this->DLLAttached) {
        FreeLibrary(hDLL);
    }

    this->DLLAttached = false;
}

void PlantEmulator::InitializePlant(const string& filePath, int plantNumber)
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

datastruct4& PlantEmulator::getData()
{
    return this->Data4;
}

datastruct4* PlantEmulator::getpData()
{
    return &this->Data4;
}

void PlantEmulator::LaunchConsumerThread(thread &t1) {
    t1 = thread(&PlantEmulator::ConsumerThreadFun, this, &cDataCat);
}

void PlantEmulator::ConsumerThreadFun(ControlData *cData)
{   
    HANDLE hFile;
    hFile = CreateFileA(this->filepath, FILE_APPEND_DATA, 0,
        NULL, OPEN_EXISTING, 0, NULL);
    if (hFile == INVALID_HANDLE_VALUE) {
        cout << "Some error " << GetLastError() << endl;
    }
    while (true) {
        unique_lock<mutex> lock(cData->mx);

        if (cData->state == 's') {
            break;
        }

        if (cData->state == 'b') continue;

        cData->cv.wait(lock, [&]() { return !cData->pBuf->empty(); });

        // Check if there is data in pBuf
        if (!cData->pBuf->empty()) {

            DataManipulation::ParseData4(cData->pBuf, &this->Data4, chrono::system_clock::now(), true);
            DataManipulation::WriteData4ToFile(hFile, cData->pBuf);

            cData->pBuf->clear();
            //this_thread::sleep_for(chrono::seconds(1)); // Use if too terminal window updates too much
        }
        cData->cv.notify_one();
    }
    CloseHandle(hFile);
    LOG("Exiting consumer")
}



