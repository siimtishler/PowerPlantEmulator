#pragma once
#include <windows.h>
#include <thread>

#include "common.h"
#include "PlantEmulator.h"

class DataManipulation
{
public:

	static void getUnitAndPrecision(std::string, std::string*, int*);

	static void AddData4(const std::string&, const std::string&, 
		const std::variant<int, double>&, datastruct4 *, std::chrono::system_clock::time_point);

	static void PrintData4ChP(const datastruct4 &, const std::string&, const std::string&);

	static void ParseData4(std::vector<unsigned char>*, datastruct4*, 
		std::chrono::system_clock::time_point = std::chrono::system_clock::now(), boolean = true);

	static void WriteData4ToFile(HANDLE&, std::vector<unsigned char>*);

	static void ReadData4FromFile(const char*, datastruct4*);

	static void PrintMinValue(const datastruct4& Data4, const std::string&, const std::string&);
											   
	static void PrintMaxValue(const datastruct4& Data4, const std::string&, const std::string&);

};

