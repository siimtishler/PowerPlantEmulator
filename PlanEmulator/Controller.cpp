#include "Controller.h"
//#include "PlantEmulator.h"
#include <iostream>
#include <Windows.h>
#include <conio.h>
#include <thread>

using namespace std;

Controller::Controller(ControlData& cD, const char* fpath) : PlantEmulator(cD, fpath), cData(cD) {
    if (cData.pBuf == nullptr) {
        cout << "Allocating controller buffer";
        cData.pBuf = new std::vector<unsigned char>;
    }
}

Controller::~Controller() {
    if (this->cData.pBuf) {
        LOG("Deleting buffer");
        delete this->cData.pBuf;
    }
    if (this->cData.pProm) {
        LOG("Deleting promise");
        delete this->cData.pProm;
    }
}


void Controller::controllerThreadFun()
{
	while (true) {
		char buf[100];
		cin >> buf;
		cout << endl;

        std::transform(buf, buf + strlen(buf), buf, ::tolower);

        // Switch based on the command
        switch (hash(buf)) {
            //1.	“connect”: the logger attaches the DLL and calls method
            //		SetIAS0410PlantEmulator().
            case hash("connect"):
                Connect();
                break;

                //2.	“disconnect” : the logger detaches the DLL. If the producer thread is running, this
                //		command should be ignored.
            case hash("disconnect"):
                Disconnect();
                break;

                //3.	"start” : the logger sets the state to ‘r’, clears the contents of buffer, launches the
                //		consumer thread and calls RunIAS0410PlantEmulator(). The emulator starts to
                //		generate data. The logger must add timestamp, show the measurement data on
                //		screen and store in data structure and in log file.
            case hash("start"):
                Start();
                break;

                //4.	“stop” : the logger sets the state to ‘s’, thus terminating the producer and
                //		consumer threads. After that, the operator may disconnect the DLL or restart the
                //		data generating.
            case hash("stop"):
                Stop();
                break;

                //5.	“break” : the logger sets the state to ‘b’. The producer and consumer threads
                //		continue to run but the data is not generated.
            case hash("break"):
                Break();
                break;

                //6.	“resume” : the logger sets the state back ‘r’, normal working continues
            case hash("resume"):
                Resume();
                break;

                //7.	“print” : the logger shows the contents of the data structure in the Windows command
                //		prompt window. If the DLL is attached, this command should be ignored.
            case hash("print"):
                Print();
                break;

                //8.	“print channel_name” : the logger retrieves from the data structure all the data served
                //		by the specified channel and shows it in the Windows command prompt window. If
                //		the DLL is attached, this command should be ignored. Example : “print Ch2”.
            case hash("print ch"):
                PrintCh();
                break;

                //9.	“print channel_name point_name” : the logger retrieves from the data structure all the
                //		data served by the specified channel and specified measurement point and shows it
                //		in the Windows command prompt window. If the DLL is attached, this command
                //		should be ignored. Example : “print Ch2 P4”.
            case hash("print ch p"):
                PrintChP();
                break;

                //10.	“limits channel_name point_name” : the logger finds from the data structure the
            case hash("limits"):

                // Should check if we have anything in the data structure first
                // Or maybe this checks the file and if the file has any contents
                Limits();
                break;

                // 11.	“exit”: must be applicable at any moment. If necessary terminates the threads,
                //		closes the log file, detaches the DLL, clears everything and quits.
            case hash("exit"):
                Exit();
                //this->consumerT.join();
                return;

            case hash("state"):
                cout << "Current state: " << cData.state << endl;
                //this->consumerT.join();
                break;
        }
	}
}

void Controller::deletecData() {
    ControlData* cD = &cData;
    if (cD) {
        LOG("DELETING");
        if(cD->pBuf) delete cD->pBuf;
        if (cD->pProm) delete cD->pProm;
        delete cD;
        cD = nullptr;
    }
}

void Controller::initcData() {
    if (!this->cData.pBuf->empty()) {
        LOG("Buffer not empty, clearing");
        this->cData.pBuf->clear();
    }

    this->cData.pBuf = new std::vector<unsigned char>;
    this->cData.pProm = new std::promise<void>;
}

bool Controller::producerRunning()
{
    return (cData.state == 'r');
}

bool Controller::producerBreaked()
{
    return (cData.state == 'b');
}

void Controller::Connect()
{
    if (isDLLAttached()) {
        LOG("Cant attach DLL, DLL already attached");
        return;
    }
    try {
        this->LoadDLL();
        this->InitializePlant("C:\\Users\\S11M\\Desktop\\IAS0410Plants.txt", 6);

    }
    catch(const runtime_error &e){
        LOG(e.what());
        cerr << "Couldn't Load and Init plant" << endl;
    }
    LOG("DLL Connected");
}

void Controller::Disconnect()
{
    if (!isDLLAttached()) {
        LOG("Cannot disconnect, DLL not attached");
        return;
    }
    if (producerRunning() || producerBreaked()) {
        LOG("Cannot disconnect, producer running/breaked");
        return;
    }

    else {
        this->DisconnectDLL();
        LOG("DLL Disconnected");
    }
}

void Controller::Start()
{
    if (producerRunning()) {
        LOG("Wont start, producer already running");
        return;
    }
    else if (!isDLLAttached()) {
        LOG("Wont start, DLL not attached");
        return;

    }

    this->initcData();
    this->cData.state = 'r';
    // Launches producer thread
    try {
        this->RunPlant();
        LOG("Produced started");

    }
    catch (const runtime_error &e) {
        LOG(e.what());
        cerr << "Couldn't run plant" << endl;
    }

    // Launches consumer thread
    this->LaunchConsumerThread(this->consumerT);
    LOG("Consumer started");
}

void Controller::Stop()
{
    if (!isDLLAttached()) {
        LOG("Can't stop, DLL not attached");
        return;
    }
    if (!producerRunning() && !producerBreaked()) {
        LOG("Can't stop, producer not running/breaked");
        return;
    }

    this->cData.state = 's';
    cData.cv.notify_all();
    this->fut = this->cData.pProm->get_future();
    this->fut.wait();

    this_thread::sleep_for(chrono::milliseconds(300));

    if (this->consumerT.joinable()) {
        this->consumerT.join();
    }

    this_thread::sleep_for(chrono::milliseconds(300));

    if (!this->cData.pBuf->empty()) {
        this->cData.pBuf->clear();
    }

    LOG("Threads terminated");
}

void Controller::Break()
{
    if (!producerRunning()) {
        LOG("Can't break, producer not running");
        return;
    }
    else
    {
        this->cData.state = 'b';
        LOG("Breaked");
    }
}

void Controller::Resume()
{
    if (!producerBreaked()) {
        LOG("Can't resume, producer not breaked");
        return;
    }
    else
    {
        this->cData.state = 'r';
        LOG("Resuming");
    }
}

void Controller::Print()
{
    if (isDLLAttached()) {
        LOG("Can't print, DLL attached");
        return;
    }
    else
    {
        LOG("Print from struct or file");
        this->printData4();
        return;
        DataManipulation::PrintData4(this->getData());
    }
}

void Controller::PrintCh()
{
    if (isDLLAttached()) {
        LOG("Can't print, DLL attached");
        return;
    }
    else
    {
        LOG("Print from data struct. Example: “print Ch2”");
    }

}

void Controller::PrintChP()
{
    if (isDLLAttached()) {
        LOG("Can't print, DLL attached");
        return;
    }
    else
    {
        LOG("Print from data struct. Example: “print Ch2 P4”");
    }
}

void Controller::Limits()
{
    LOG("Print limits");
}

void Controller::Exit()
{
    Stop();
    Disconnect();
    LOG("Exiting logger");
    return;
}
