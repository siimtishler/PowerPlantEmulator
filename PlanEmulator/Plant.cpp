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

map<string, map<string, list<pair<variant<int, double>, chrono::system_clock::time_point> >* >* > Data4;


void WriteDataToFile(HANDLE& hFile, vector<unsigned char>* pBuf) {
	if (hFile == INVALID_HANDLE_VALUE || pBuf == nullptr) {
		cerr << "Invalid parameters." << endl;
		return;
	}

	auto now = chrono::system_clock::now();

	/*auto timestamp = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()).count();

	vector<unsigned char> timestampBytes(reinterpret_cast<unsigned char*>(&timestamp),
		reinterpret_cast<unsigned char*>(&timestamp) + sizeof(timestamp));

	pBuf->insert(pBuf->end(), timestampBytes.begin(), timestampBytes.end());*/

	DWORD nWritten = 0;

	if (!WriteFile(hFile, pBuf->data(), static_cast<DWORD>(pBuf->size()), &nWritten, NULL)) {
		cerr << "Error writing to file " << GetLastError() << endl;
		return;
	}

	cout << "SIZE OF TIMESTAMP: " << sizeof(now) << endl;
	if (!WriteFile(hFile, &now, sizeof(now), &nWritten, NULL)) {
		cerr << "Error writing to file " << GetLastError() << endl;
		return;
	}

	cout << "Data written to file with timestamp." << endl;
}

void readAndPrintTimestamp(const string& filename) {
	// Open the file in binary mode for reading
	ifstream file(filename, ios::binary);

	if (!file.is_open()) {
		cerr << "Error opening file for reading." << endl;
		return;
	}

	// Read the length of the data
	int dataLength;
	file.read(reinterpret_cast<char*>(&dataLength), sizeof(dataLength));

	file.seekg(0, ios::beg);

	int totalData = (int)dataLength + 1;

	vector<unsigned char> buffer(totalData);

	cout << "Total data " << totalData << endl;

	// Read the data (excluding the length byte)
	file.read(reinterpret_cast<char*>(buffer.data()), totalData);

	// Close the file
	file.close();

	// Print the data and timestamp
	cout << "Data read from file: ";
	for (unsigned char byte : buffer) {
		cout << hex << static_cast<int>(byte) << " ";
	}
	cout << endl;

	// Print the timestamp using put_time
	chrono::system_clock::time_point timestamp;
	memcpy(&timestamp, buffer.data() + totalData - 8 , sizeof(timestamp));

	time_t timestamp_t = chrono::system_clock::to_time_t(timestamp);
	tm timestamp_tm = *localtime(&timestamp_t);

	cout << "Timestamp read from file: ";
	cout << put_time(&timestamp_tm, "%Y-%m-%d %H:%M:%S") << endl;
}


int main(int argc, char **argv)
{

	//HANDLE hFile2 = CreateFileA("C:\\Users\\S11M\\Desktop\\test123.bin", GENERIC_READ | GENERIC_WRITE, 0,
	//	NULL, CREATE_ALWAYS, 0, NULL);


	//if (hFile2 == INVALID_HANDLE_VALUE) {
	//	cerr << "Error creating file " << GetLastError() << endl;
	//	return 1;
	//}

	//vector<unsigned char> dataToWrite = { 3, 2, 3 };

	//// Write data with timestamp to the file
	//WriteDataToFile(hFile2, &dataToWrite);

	//CloseHandle(hFile2);
	// Read and print the timestamp from the file
	//readAndPrintTimestamp("C:\\Users\\S11M\\Desktop\\test123.bin");

	//return 1;
	thread prod;
	thread cons;

	future<void> fut;

	const char* filepath = "C:\\Users\\S11M\\Desktop\\IAS0410Plants.txt";
	int plantNum = 6;

	ControlData cData2;
	
	cData2.pBuf = new vector<unsigned char>;
	cData2.pProm = new promise<void>;

	Controller contr(cData2, argv[1]);

	/*HANDLE hFile;
	hFile = CreateFileA("Data44.bin", GENERIC_READ | GENERIC_WRITE, 0,
		NULL, OPEN_ALWAYS, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		cout << "Some error " << GetLastError() << endl;
	}

	DataManipulation::ReadData4FromFile(hFile);

	CloseHandle(hFile);*/

	try {
		contr.controllerThreadFun();
	}
	catch (exception& e) {
		cerr << "Error" << e.what() << endl;
		return -1;
	}


	return 1;

	// Add later

	//UINT WINAPI codepage = GetConsoleOutputCP();
	//SetConsoleOutputCP(1252);
	//SetConsoleOutputCP(codepage);
}