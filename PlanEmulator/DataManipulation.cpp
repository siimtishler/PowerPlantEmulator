#include "DataManipulation.h"
#include <iostream>
#include <string>
#include <sstream>
#include <chrono> 
#include <iomanip>
#include <fstream>
#include "common.h"
#include <limits>

#pragma warning(disable: 4996)

using namespace std;

void DataManipulation::getUnitAndPrecision(std::string point, std::string* unit, int* precision)
{
    if (point.find("pH") != string::npos) {
        *precision = 1;
        *unit = "";
    }
    if (point.find("flow") != string::npos) {
        *precision = 3;
        *unit = "m³/s";
    }
    if (point.find("Level") != string::npos) {
        *precision = 0;
        *unit = "%";
    }
    if (point.find("temperature") != string::npos) {
        *precision = 1;
        *unit = "°C";
    }
    if (point.find("concentration") != string::npos) {
        *precision = 0;
        *unit = "%";
    }
    if (point.find("conductivity") != string::npos) {
        *precision = 2;
        *unit = "s/M";
    }
}

void DataManipulation::AddData4(const string& channelName, const string& pointName,
                                const variant<int, double>& value, datastruct4 * pData4, chrono::system_clock::time_point timepoint)
{
    auto channelIt = pData4->find(channelName);

    if (channelIt == pData4->end()) {
        // Channel doesn't exist, create a new inner map
        auto innerMap = new map<string, list<pair<variant<int, double>, chrono::system_clock::time_point>>*>;
        (*pData4)[channelName] = innerMap;
    }

    // Access the inner map for the channel
    auto innerMap = (*pData4)[channelName];

    // Check if the point already exists
    auto pointIt = innerMap->find(pointName);

    if (pointIt == innerMap->end()) {
        // Point doesn't exist, create a new list
        auto dataList = new list<pair<variant<int, double>, chrono::system_clock::time_point>>;
        (*innerMap)[pointName] = dataList;
    }

    // Access the list for the point
    auto& dataList = *(*innerMap)[pointName];

    // Add the data to the list
    dataList.push_back({ value, timepoint });
    //dataList.push_back({ value, timepoint });
}

void DataManipulation::PrintMinValue(const datastruct4& Data4, const string& ch, const string& p)
{
    int minValueInt = INT_MAX;
    double minValueDouble = DBL_MAX;

    bool intUsed = true;
    bool gotVal = false;

    string channelName;
    struct tm now_tm;
    string unit;
    int precision = 0;

    for (const auto& channelEntry : Data4) {
        if (channelEntry.first != ch && !ch.empty()) continue;

        for (const auto& pointEntry : *channelEntry.second) {
            if (pointEntry.first != p && !p.empty()) continue;


            DataManipulation::getUnitAndPrecision(pointEntry.first, &unit, &precision);

            for (const auto& dataEntry : *pointEntry.second) {
                // Print the value based on its type
                if (dataEntry.first.index() == 0) {
                    int measurment = get<int>(dataEntry.first);
                    gotVal = true;
                    if (measurment < minValueInt) {
                        time_t now_t = chrono::system_clock::to_time_t(dataEntry.second);
                        localtime_s(&now_tm, &now_t);
                        minValueInt = measurment;
                        intUsed = true;
                    }
                }
                else if (dataEntry.first.index() == 1) {
                    double measurment = get<double>(dataEntry.first);
                    gotVal = true;
                    if (measurment < minValueDouble) {
                        time_t now_t = chrono::system_clock::to_time_t(dataEntry.second);
                        localtime_s(&now_tm, &now_t);
                        minValueDouble = measurment;
                        intUsed = false;
                    }
                }
            }
        }
        cout << endl;
    }

    if (!gotVal) {
        LOG("Wrong channel or point name");
        return;
    }
    
    if (intUsed) {
        cout << "Min value : " << setprecision(precision) << minValueInt << " " << unit << "\t" << put_time(&now_tm, "%d-%m-%Y %H:%M:%S ");
    }
    else {
        cout << "Min value : " << fixed << setprecision(precision) << minValueDouble << " " << unit << "\t" << put_time(&now_tm, "%d-%m-%Y %H:%M:%S ");
    }
}

void DataManipulation::PrintMaxValue(const datastruct4& Data4, const string& ch, const string& p)
{
    int maxValueInt = 0;
    double maxValueDouble = 0;

    bool intUsed = true;
    bool gotVal = false;

    bool flagfirst = true;
    struct tm now_tm;
    string unit;
    int precision = 0;

    for (const auto& channelEntry : Data4) {
        if (channelEntry.first != ch && !ch.empty()) continue;

        for (const auto& pointEntry : *channelEntry.second) {
            if (pointEntry.first != p && !p.empty()) continue;

            DataManipulation::getUnitAndPrecision(pointEntry.first, &unit, &precision);

            for (const auto& dataEntry : *pointEntry.second) {
                // Print the value based on its type
                if (dataEntry.first.index() == 0) {
                    int measurment = get<int>(dataEntry.first);
                    gotVal = true;
                    if (flagfirst) {
                        maxValueInt = measurment;
                        flagfirst = false;
                    }

                    if (measurment > maxValueInt) {
                        maxValueInt = measurment;
                        intUsed = true;
                    }
                }
                else if (dataEntry.first.index() == 1) {
                    double measurment = get<double>(dataEntry.first);
                    gotVal = true;
                    if (flagfirst) {
                        time_t now_t = chrono::system_clock::to_time_t(dataEntry.second);
                        localtime_s(&now_tm, &now_t);
                        maxValueDouble = measurment;
                        flagfirst = false;
                    }

                    if (measurment > maxValueDouble) {
                        time_t now_t = chrono::system_clock::to_time_t(dataEntry.second);
                        localtime_s(&now_tm, &now_t);
                        maxValueDouble = measurment;
                        intUsed = false;
                    }
                    
                }
            }
        }
        cout << endl;
    }

    if (!gotVal) {
        LOG("Wrong channel or point name");
        return;
    }

    if (intUsed) {
        cout << "Max value : " << setprecision(precision) << maxValueInt << " " << unit <<  "\t" << put_time(&now_tm, "%d-%m-%Y %H:%M:%S ");
    }
    else {
        cout << "Max value : " << fixed << setprecision(precision) << maxValueDouble << " " << unit << "\t"<< put_time(&now_tm, "%d-%m-%Y %H:%M:%S ");
    }
}

void DataManipulation::PrintData4ChP(const datastruct4& Data4, const string& ch, const string& p)
{
    for (const auto& channelEntry : Data4) {
        if (channelEntry.first != ch && !ch.empty()) continue;
        cout << "Ch:" << channelEntry.first << endl;

        for (const auto& pointEntry : *channelEntry.second) {
            if (pointEntry.first != p && !p.empty()) continue;

            string unit;
            int precision = 0;

            DataManipulation::getUnitAndPrecision(pointEntry.first, &unit, &precision);

            for (const auto& dataEntry : *pointEntry.second) {
                // Print the value based on its type
                if (dataEntry.first.index() == 0) {
                    cout << setprecision(precision) << pointEntry.first << " = " << get<int>(dataEntry.first) << " " << unit <<"\t";
                }
                else if (dataEntry.first.index() == 1) {
                    cout << fixed << setprecision(precision) << pointEntry.first << " = " << get<double>(dataEntry.first) << " " << unit << "\t";
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

void DataManipulation::ParseData4(vector<unsigned char>* pBuf, datastruct4* pData4, chrono::system_clock::time_point timestamp, boolean flag_print)
{
    vector<unsigned char>& receivedData = *(pBuf);

    vector<unsigned char> temp = receivedData;

    // First 4 bytes
    int packageLength = *reinterpret_cast<int*>(&receivedData[0]);

    // Bytes 4-7
    int numChannels = *reinterpret_cast<int*>(&receivedData[4]);

    stringstream sout;

    time_t timestamp_t = chrono::system_clock::to_time_t(timestamp);
    tm timestamp_tm = *localtime(&timestamp_t);
    sout << put_time(&timestamp_tm, "%Y-%m-%d %H:%M:%S") << endl;

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

            transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

            bool useInt = lower.find("level") != string::npos || lower.find("concentration") != string::npos;

            double measuredValue;

            if (useInt) {
                measuredValue = *reinterpret_cast<int*>(&receivedData[currentIndex]);
                currentIndex += sizeof(int);
            }
            else {
                measuredValue = *reinterpret_cast<double*>(&receivedData[currentIndex]);
                currentIndex += sizeof(double);
            }
            string unit;
            int precision;

            DataManipulation::getUnitAndPrecision(pointName, &unit, &precision);

            DataManipulation::AddData4(channelName, pointName, measuredValue, pData4, timestamp);

            //sout << pointName << " = " << measuredValue << endl;

            sout << fixed << setprecision(precision) << pointName << " = " << measuredValue << " " << unit << endl;
        }
    }
    sout << "Package len: " << packageLength << " Index: " << currentIndex << endl << endl;
    if (flag_print) cout << sout.str();
}

void DataManipulation::WriteData4ToFile(HANDLE& hFile, vector<unsigned char>* pBuf)
{
    static int totalMem = 0, count = 0;

    stringstream sout;

    if (hFile == INVALID_HANDLE_VALUE || pBuf == nullptr) {
        cerr << "Invalid parameters." << endl;
        return;
    }

    DWORD nWritten = 0;
    int nToWrite = 1;

    auto now = chrono::system_clock::now();

    int packageLength = *reinterpret_cast<int*>(&pBuf->at(0));
    sout << "pac len: " << packageLength << endl;
    *reinterpret_cast<int*>(&pBuf->at(0)) = packageLength + sizeof(now);
    packageLength = *reinterpret_cast<int*>(&pBuf->at(0));
    sout << "pac len: " << packageLength << endl;


    if (!WriteFile(hFile, pBuf->data(), (DWORD)pBuf->size(), &nWritten, NULL)) {
        cerr << "Error writing to file " << GetLastError() << endl;
        return;
    }

    sout << "SIZE OF TIMESTAMP: " << sizeof(now) << endl;
    if (!WriteFile(hFile, &now, sizeof(now), &nWritten, NULL)) {
        cerr << "Error writing to file " << GetLastError() << endl;
        return;
    }

    sout << "package write: " << count++ << endl;

    sout << "WRITING: " << endl;
    for_each(pBuf->begin(), pBuf->end(), [&](int i) {sout << hex << i << " "; });
    sout << dec;
    sout << endl;
    //cout << sout.str();
}

void DataManipulation::ReadData4FromFile(const char *filepath, datastruct4 *pData4)
{
    if (!filepath) {
        cerr << "Invalid file" << endl;
        return;
    }

    ifstream file(filepath, ios::binary);

    if (!file.is_open()) {
        cout << "File didn't open" << endl;
        return;
    }
    file.seekg(0, ios::beg);

    static int count = 0;
    count = 0;
    while (!file.eof()) {
        // Get the current position of the file cursor
        streampos currentPosition = file.tellg();
        stringstream sout;
        sout << dec << "Current position: " << currentPosition << endl;

        // Read the size of the chunk
        int chunkSize;
        file.read(reinterpret_cast<char*>(&chunkSize), sizeof(int));
        sout << "Read value: " << chunkSize << endl;

        if (chunkSize <= 0) {
            break;
        }
        chunkSize += 1;
        // Move cursor to start of chunk
        file.seekg(currentPosition, ios::beg);

        // Read entire chunk
        vector<unsigned char> buffer(chunkSize);
        file.read(reinterpret_cast<char*>(buffer.data()), chunkSize);

        // Get new position of file cursor
        currentPosition = file.tellg();
        if (currentPosition == -1) break;
        sout << "New position: " << currentPosition << endl;

        sout << "Read " << chunkSize << " bytes: ";
        for (char byte : buffer) {
            sout << hex << static_cast<int>(static_cast<unsigned char>(byte)) << " ";
        }
        sout << endl;

        chrono::system_clock::time_point timestamp;
        memcpy(&timestamp, buffer.data() + chunkSize - 8, sizeof(timestamp));

        time_t timestamp_t = chrono::system_clock::to_time_t(timestamp);
        tm timestamp_tm = *localtime(&timestamp_t);

        sout << "Timestamp read from file: ";
        sout << put_time(&timestamp_tm, "%Y-%m-%d %H:%M:%S") << endl;

        sout << "package read: " << count++ << endl;

        //cout << sout.str();
        DataManipulation::ParseData4(&buffer, pData4, timestamp, false);
    }
    file.seekg(0, ios::beg);
    file.close();
    return;
}
