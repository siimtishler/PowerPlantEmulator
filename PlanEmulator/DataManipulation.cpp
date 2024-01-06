#include "DataManipulation.h"
#include <iostream>
#include <string>
#include <sstream>
#include <chrono> 
#include <iomanip>
#include <fstream>

#pragma warning(disable: 4996)

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
    vector<unsigned char>& receivedData = *(pBuf);

    // First 4 bytes
    int packageLength = *reinterpret_cast<int*>(&receivedData[0]);

    //cout << "PACKAGE LEN PARSE: " << packageLength << endl;

    // Bytes 4-7
    int numChannels = *reinterpret_cast<int*>(&receivedData[4]);

    stringstream sout;

    chrono::system_clock::time_point t = chrono::system_clock::now();
    time_t now_t = chrono::system_clock::to_time_t(t);
    struct tm now_tm;
    localtime_s(&now_tm, &now_t);
    cout << put_time(&now_tm, "%d-%m-%Y %H:%M:%S ");
    auto seconds = std::chrono::time_point_cast<std::chrono::seconds>(t);
    std::cout << "Time point value: " << seconds.time_since_epoch().count() << " seconds since epoch." << std::endl;
    // Print the value
    /*std::cout << "Time point value: " << seconds.time_since_epoch().count() << " seconds since epoch." << std::endl;
    sout << "Received: " << packageLength << " bytes" << endl;*/


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

                

                continue;
                chrono::system_clock::time_point tpoint;
                long tStampval = *reinterpret_cast<long*>(&receivedData[currentIndex]);
                memcpy(&tpoint, &receivedData[currentIndex], sizeof(tpoint));

                time_t now_t2 = chrono::system_clock::to_time_t(tpoint);
                struct tm now_tm2;
                localtime_s(&now_tm2, &now_t2);
                cout << put_time(&now_tm2, "%d-%m-%Y %H:%M:%S ");

                auto seconds = std::chrono::time_point_cast<std::chrono::seconds>(tpoint);

                std::cout << "Time point value: " << seconds.time_since_epoch().count() << " seconds since epoch." << std::endl;

                currentIndex += 8;
            }


            //auto seconds = std::chrono::time_point_cast<std::chrono::seconds>(tpoint);

            //// Print the value
            //std::cout << "Time point value: " << seconds.time_since_epoch().count() << " seconds since epoch." << std::endl;

            /*time_t now_t = chrono::system_clock::to_time_t(tpoint);
            struct tm now_tm;
            localtime_s(&now_tm, &now_t);
            sout << put_time(&now_tm, "%d-%m-%Y %H:%M:%S ");

            cout << sizeof(tpoint) << endl;*/

            DataManipulation::AddData4(channelName, pointName, measuredValue, pData4);

            sout << pointName << " = " << measuredValue << endl;
        }
    }
    cout << "Package len: " << packageLength << " Index: " << currentIndex << endl << endl;
    //cout << sout.str();
}

void DataManipulation::WriteData4ToFile(HANDLE& hFile, vector<unsigned char>* pBuf)
{
    static int totalMem = 0;


    if (hFile == INVALID_HANDLE_VALUE || pBuf == nullptr) {
        cerr << "Invalid parameters." << endl;
        return;
    }

    DWORD nWritten = 0;
    int nToWrite = 1;
    

    auto now = chrono::system_clock::now();
    // Convert the time to a string
    auto timestamp = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()).count();
    // Convert timestamp to a vector of unsigned char
    vector<unsigned char> timestampBytes(reinterpret_cast<unsigned char*>(&timestamp),
        reinterpret_cast<unsigned char*>(&timestamp) + sizeof(timestamp));


    //auto now = std::chrono::system_clock::now();

    // Convert the time to a string
    //auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();


    for_each(timestampBytes.begin(), timestampBytes.end(), [&](unsigned char i) { pBuf->push_back(i); });
    int packageLength = *reinterpret_cast<int*>(&pBuf->at(0));
    totalMem += pBuf->size() - 1;
    *reinterpret_cast<int*>(&pBuf->at(0)) = pBuf->size() - 1;
    packageLength = *reinterpret_cast<int*>(&pBuf->at(0));

    //SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
    if (!WriteFile(hFile, pBuf->data(), (DWORD)pBuf->size(), &nWritten, NULL)) {
        cerr << "Error writing to file " << GetLastError() << endl;
        return;
    }
    //SetEndOfFile(hFile);

    cout << "WRITING: " << endl;
    for_each(pBuf->begin(), pBuf->end(), [](int i) {cout << i << " "; });
    cout << endl;

}

void DataManipulation::ReadData4FromFile(HANDLE& hFile)
{
    if (hFile == INVALID_HANDLE_VALUE) {
        cerr << "Invalid parameters." << endl;
        return;
    }
    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

    auto now = chrono::system_clock::now();
    // Convert the time to a string
    auto timestamp = chrono::duration_cast<chrono::milliseconds>(now.time_since_epoch()).count();
    // Convert timestamp to a vector of unsigned char
    vector<unsigned char> timestampBytes(reinterpret_cast<unsigned char*>(&timestamp),
        reinterpret_cast<unsigned char*>(&timestamp) + sizeof(timestamp));

    for_each(timestampBytes.begin(), timestampBytes.end(), [](int i) { cout << hex << i << " "; });

    cout << dec;
    unsigned long alreadyRead = 0, toRead = sizeof(unsigned int);
    unsigned int packageLen = 0;

    bool reading = true;
    int prevPointer = 0;

    int pointer = 0;

    //while (ReadFile(hFile, &packageLen, toRead, &alreadyRead, NULL) && alreadyRead == toRead) {
    //    cout << "Package len: " << hex << packageLen << " READING:" << endl;

    //    // Move the pointer to the beginning of the next package
    //    SetFilePointer(hFile, 0, NULL, FILE_CURRENT);

    //    vector<unsigned char> packageData(toRead);

    //   

    //    ReadFile(hFile, packageData.data(), packageLen, &alreadyRead, NULL);

    //    for_each(packageData.begin(), packageData.end(), [](int i) {cout << hex << i << " "; });

    //    /*ReadFile(hFile, &packageLen, toRead, &alreadyRead, NULL);
    //    cout << "Package len: " << packageLen << " READING:" << endl;*/
    //    break;
    //}

    //if (GetLastError() != ERROR_SUCCESS) {
    //    cout << "Error reading file: " << GetLastError() << endl;
    //}

    //while (reading) {
    //    int result = ReadFile(hFile, &packageLen, toRead, &alreadyRead, NULL);
    //    if (!result) {
    //        cout << "Data not read, error " << GetLastError() << endl;
    //        reading = false;
    //    }
    //    else if (alreadyRead != toRead) {
    //        cout << "Only " << alreadyRead << " bytes instead of " << toRead << " was read" << endl;
    //        reading = false;
    //    }    

    //    cout << "Package len: " << packageLen << " READING:" << endl;

    //    pointer += packageLen;

    //    SetFilePointer(hFile, packageLen, NULL, FILE_CURRENT);

    //    // Check if we have reached the end of the file
    //    if (GetLastError() == ERROR_SUCCESS) {
    //        // End of file reached
    //        break;
    //    }


    toRead = packageLen;
    alreadyRead = 0;


    vector<unsigned char> packageData(toRead);


    //int result = ReadFile(hFile, packageData.data(), toRead, &alreadyRead, NULL);
    //if (!result) {
    //    cout << "Data not read, error " << GetLastError() << endl;
    //    reading = false;
    //}
    //else if (alreadyRead != toRead) {
    //    cout << "Only " << alreadyRead << " bytes instead of " << toRead << " was read" << endl;
    //    reading = false;
    //}
    //else {
    //    for_each(packageData.begin(), packageData.end(), [](int i) {cout << i << " "; });
    //    cout << endl;
    //}
    //prevPointer += toRead + 1;
    
    ifstream file("C:\\Users\\S11M\\Desktop\\Data44.bin", ios::binary);

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
        sout << "Read int value: " << chunkSize << endl;

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
        datastruct4 data4;
        DataManipulation::ParseData4(&buffer, &data4);
        //cout << sout.str();
    }

    file.close();

    return;

    ////////

    streampos currentPosition = file.tellg();
    cout << "Current position: " << currentPosition << endl;

    int arraySize;
    file.read(reinterpret_cast<char*>(&arraySize), sizeof(int));
    cout << "Read int value: " << arraySize << endl;

    // Move the cursor back to the start
    file.seekg(currentPosition, ios::beg);

    // Get the new position of the file cursor
    currentPosition = file.tellg();
    cout << "New position: " << currentPosition << endl;

    arraySize += 1;

    vector<char> buffer(arraySize);
    file.read(buffer.data(), arraySize);

    cout << "Read " << arraySize << " bytes: ";
    for (char byte : buffer) {
        cout << hex << static_cast<int>(static_cast<unsigned char>(byte)) << " ";
    }
    cout << endl;

    file.close();

    return;

    currentPosition = file.tellg();
    cout << dec << "Current position: " << currentPosition << endl;

    file.read(reinterpret_cast<char*>(&arraySize), sizeof(int));
    cout << "Read int value: " << arraySize << endl;

    while (ReadFile(hFile, &packageLen, toRead, &alreadyRead, NULL)) {

        if (alreadyRead != toRead) {
            cout << "Only " << alreadyRead << " bytes instead of " << toRead << " was read" << endl;
            break;
        }
        else {
            cout << alreadyRead << " bytes was read" << endl;
            cout << "Package len " << packageLen << endl;

            toRead = packageLen;
            alreadyRead = 0;
            vector<unsigned char> packageData(toRead);

            if (ReadFile(hFile, packageData.data(), toRead, &alreadyRead, NULL)) {
                // Process packageData as needed
                // ...
                for_each(packageData.begin(), packageData.end(), [](int i) {cout << hex << i << " "; });
                cout << endl;
                // Move the file cursor to the next package
                SetFilePointer(hFile, packageLen, NULL, FILE_CURRENT);
            }
            else {
                cout << "Error reading package data. Error code: " << GetLastError() << endl;
                break;
            }

            // Reset toRead for the next package length
            toRead = sizeof(unsigned int);
        }
    }

   
}


void DataManipulation::ParseData4File(vector<unsigned char>* pBuf, datastruct4* pData4)
{
    vector<unsigned char>& receivedData = *(pBuf);

    // First 4 bytes
    int packageLength = *reinterpret_cast<int*>(&receivedData[0]);

    cout << "PACKAGE LEN PARSE: " << packageLength << endl;

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
