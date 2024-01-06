#include "DataManipulation.h"
#include <iostream>
#include <string>
#include <sstream>
#include <chrono> 
#include <iomanip>

using namespace std;

void DataManipulation::AddData4(const string& channelName, const string& pointName, 
                                const variant<int, double>& value, datastruct4 * pData4)
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
    dataList.push_back({ value, chrono::system_clock::now() });
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

void DataManipulation::ParseData4(vector<unsigned char>* pBuf, datastruct4* pData4)
{
    for_each(pBuf->begin(), pBuf->begin() + 7, [](int i) { cout << i << " "; });
    vector<unsigned char>& receivedData = *(pBuf);

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

            DataManipulation::AddData4(channelName, pointName, measuredValue, pData4);

            sout << pointName << " = " << measuredValue << endl;
        }
    }
    sout << "Package len: " << packageLength << " Index: " << currentIndex << endl << endl;
    //cout << sout.str();
}

void DataManipulation::WriteData4ToFile(HANDLE& hFile, vector<unsigned char>* pBuf)
{
    if (hFile == INVALID_HANDLE_VALUE || pBuf == nullptr) {
        cerr << "Invalid parameters." << endl;
        return;
    }

    DWORD nWritten = 0;
    int nToWrite = 1;

    for_each(pBuf->begin(), pBuf->end(), [](int i) {cout << i << " "; });
    cout << endl;
    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
    if (!WriteFile(hFile, pBuf->data(), (DWORD)pBuf->size(), &nWritten, NULL)) {
        cerr << "Error writing to file " << GetLastError() << endl;
        return;
    }

    DataManipulation::ReadData4FromFile(hFile);

    //auto now = chrono::system_clock::now();
    //// Convert the time to a string
    //auto timestamp = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()).count();
    //// Convert timestamp to a vector of unsigned char
    //vector<unsigned char> timestampBytes(reinterpret_cast<unsigned char*>(&timestamp),
    //    reinterpret_cast<unsigned char*>(&timestamp) + sizeof(timestamp));
    //cout << pBuf->size() << endl;
    //for_each(timestampBytes.begin(), timestampBytes.end(), [&](unsigned char i) { pBuf->push_back(i); });
    //cout << pBuf->size() << endl;
    //int packageLength = *reinterpret_cast<int*>(&pBuf->at(0));
    //cout << "Package length: " << packageLength << endl;
    //*reinterpret_cast<int*>(&pBuf->at(0)) = pBuf->size();
    //packageLength = *reinterpret_cast<int*>(&pBuf->at(0));
    //cout << "New package length: " << packageLength << endl;


}

void DataManipulation::ReadData4FromFile(HANDLE& hFile)
{
    if (hFile == INVALID_HANDLE_VALUE) {
        cerr << "Invalid parameters." << endl;
        return;
    }
    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

    unsigned long alreadyRead = 0, toRead = sizeof(unsigned int);
    unsigned int packageLen = 0;

    int result = ReadFile(hFile, &packageLen, toRead, &alreadyRead, NULL);
    if (!result)
        cout << "Data not read, error " << GetLastError() << endl;
    else if (alreadyRead != toRead)
        cout << "Only " << alreadyRead << " bytes instead of " << toRead << " was read"
        << endl;

    cout << "Package len: " << packageLen << endl;

    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

    //toRead = packageLen;
    //alreadyRead = 0;

    //vector<unsigned char> packageData(toRead);


    //result = ReadFile(hFile, packageData.data(), toRead, &alreadyRead, NULL);
    //if (!result)
    //    cout << "Data not read, error " << GetLastError() << endl;
    //else if (alreadyRead != toRead)
    //    cout << "Only " << alreadyRead << " bytes instead of " << toRead << " was read"
    //    << endl;
    //else {
    //    for_each(packageData.begin(), packageData.end(), [](int i) {cout << i << " "; });
    //    cout << endl;
    //}

    //while (ReadFile(hFile, &packageLen, toRead, &alreadyRead, NULL)) {

    //    if (alreadyRead != toRead) {
    //        cout << "Only " << alreadyRead << " bytes instead of " << toRead << " was read" << endl;
    //        break;
    //    }
    //    else {
    //        cout << alreadyRead << " bytes was read" << endl;
    //        cout << "Package len " << packageLen << endl;

    //        toRead = packageLen;
    //        alreadyRead = 0;
    //        vector<unsigned char> packageData(toRead);

    //        if (ReadFile(hFile, packageData.data(), toRead, &alreadyRead, NULL)) {
    //            // Process packageData as needed
    //            // ...
    //            for_each(packageData.begin(), packageData.end(), [](int i) {cout << i << " "; });
    //            cout << endl;
    //            // Move the file cursor to the next package
    //            SetFilePointer(hFile, packageLen, NULL, FILE_CURRENT);
    //        }
    //        else {
    //            cout << "Error reading package data. Error code: " << GetLastError() << endl;
    //            break;
    //        }

    //        // Reset toRead for the next package length
    //        toRead = sizeof(unsigned int);
    //    }
    //}

   
}
