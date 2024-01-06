#include "DataManipulation.h"
#include <iostream>
#include <string>
#include <sstream>
#include <chrono> 
#include <iomanip>
#include <fstream>
#include "common.h"

#pragma warning(disable: 4996)

using namespace std;

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

void DataManipulation::PrintData4(const datastruct4& Data4)
{

    for (const auto& channelEntry : Data4) {
        cout << "Ch:" << channelEntry.first << endl;

        for (const auto& pointEntry : *channelEntry.second) {

            for (const auto& dataEntry : *pointEntry.second) {
                // Print the value based on its type
                if (dataEntry.first.index() == 0) {
                    cout << pointEntry.first << " = " << get<int>(dataEntry.first) << "\t";
                }
                else if (dataEntry.first.index() == 1) {
                    cout << pointEntry.first << " = " << get<double>(dataEntry.first) << "\t";
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

            if (currentIndex == packageLength - 8) {
                currentIndex += 8;
                continue;
            }

            DataManipulation::AddData4(channelName, pointName, measuredValue, pData4, timestamp);

            sout << pointName << " = " << measuredValue << endl;
        }
    }
    sout << "Package len: " << packageLength << " Index: " << currentIndex << endl << endl;
    if (flag_print) cout << sout.str();
}

void DataManipulation::WriteData4ToFile(HANDLE& hFile, vector<unsigned char>* pBuf)
{
    static int totalMem = 0;

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
            // End of file or invalid chunk size, break out of the loop
            break;
        }
        chunkSize += 1;
        // Move the cursor back to the start of the chunk
        file.seekg(currentPosition, ios::beg);

        // Get the new position of the file cursor
        currentPosition = file.tellg();
        if (currentPosition == -1) break;
        sout << "New position: " << currentPosition << endl;

        // Read the entire chunk
        vector<unsigned char> buffer(chunkSize);
        file.read(reinterpret_cast<char*>(buffer.data()), chunkSize);

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

        //cout << sout.str();
        DataManipulation::ParseData4(&buffer, pData4, timestamp, false);
    }

    file.close();
    return;
}
