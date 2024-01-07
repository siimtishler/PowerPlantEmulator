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

//HMODULE hDLL = nullptr;
//
//ControlData* cData = nullptr;
//
//map<string, map<string, list<pair<variant<int, double>, chrono::system_clock::time_point> >* >* > Data4;

int main(int argc, char **argv)
{

	UINT WINAPI codepage = GetConsoleOutputCP();
	SetConsoleOutputCP(1252);

	thread prod;
	thread cons;

	future<void> fut;

	const char* filepath = "C:\\Users\\S11M\\Desktop\\IAS0410Plants.txt";
	int plantNum = 6;

	ControlData cData2;
	
	cData2.pBuf = new vector<unsigned char>;
	cData2.pProm = new promise<void>;

	Controller contr(cData2, argv[1]);

	try {
		contr.controllerThreadFun();
	}
	catch (exception& e) {
		cerr << "Error" << e.what() << endl;
		return -1;
	}

	SetConsoleOutputCP(codepage);
	return 1;

}