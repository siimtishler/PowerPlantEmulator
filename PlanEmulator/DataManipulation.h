#pragma once
#include <windows.h>
#include <thread>

#include "common.h"
#include "PlantEmulator.h"

class DataManipulation
{
	/*std::map<std::string, std::map<std::string, std::list<std::pair<std::variant<int, double>, std::chrono::system_clock::time_point> >* >* > Data4;*/

public:

	static void AddData4(const std::string&, const std::string&, 
		const std::variant<int, double>&, datastruct4 *);

	static void PrintData4(const datastruct4 &);

	static void ParseData4(std::vector<unsigned char>*, datastruct4*);

	static void WriteData4ToFile(HANDLE&, std::vector<unsigned char>*);

	static void ReadData4FromFile(HANDLE&, datastruct4*);

	static void ReadData4FromFile(HANDLE&);
};

