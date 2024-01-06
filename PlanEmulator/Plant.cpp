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

int main(int argc, char **argv)
{
	thread prod;
	thread cons;

	future<void> fut;

	const char* filepath = "C:\\Users\\S11M\\Desktop\\IAS0410Plants.txt";
	int plantNum = 6;

	ControlData cData2;
	
	cData2.pBuf = new vector<unsigned char>;
	cData2.pProm = new promise<void>;

	Controller contr(cData2);

	HANDLE hFile;
	hFile = CreateFileA("Data44.bin", GENERIC_READ | GENERIC_WRITE, 0,
		NULL, CREATE_ALWAYS, 0, NULL);
	if (hFile == INVALID_HANDLE_VALUE) {
		cout << "Some error " << GetLastError() << endl;
	}

	DataManipulation::ReadData4FromFile(hFile);


	CloseHandle(hFile);

	return 1;

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