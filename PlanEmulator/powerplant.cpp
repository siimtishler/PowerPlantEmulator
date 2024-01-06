#include <iostream>
#include <iomanip>
#include <stdio.h>
#include "Date.h"
#include <windows.h>
#include <mutex>
#include <vector>
#include <condition_variable>
#include <future>
//#include <winsock2.h>
//#include <ws2tcpip.h>
//#include <iphlpapi.h>
//#include <winsock2.h>

#include<sstream>

#include <fstream>

//#pragma comment(lib, "ws2_32.lib")

//#include "client.h"

#define LOG(x) std::cout << x << endl;

using namespace std;

struct ControlData
{
	mutex mx;
	condition_variable cv;
	atomic<char> state;
	vector<unsigned char>* pBuf;
	promise<void>* pProm;
};


void bar(int& a) {
	static int i = 5;
	i++;
	LOG(i);
}


class person {
	char* name = NULL;
public:
	person() : name(nullptr) {};
	person(const char* n) {
		name = new char[strlen(n) + 1];
		strcpy_s(name, strlen(n) + 1, n);
	}
	person(person& p) {
		if (this->name) {
			delete(this->name);
		}
		int n = 0;
		this->name = new char[n = int(strlen(p.name)) + 1];
		strcpy_s(this->name, n, p.name);
	}
	~person() {
		if (name) {
			delete(name);
		}
		LOG("Deleting person");
	}
	const char* getName() {
		if (name) {
			return name;
		}
		return "not assigned";
	}
	void changeName(const char* n) {
		if (name) {
			delete(name);
		}
		name = new char[strlen(n) + 1];
		strcpy_s(name, strlen(n) + 1, n);
	}
	static friend ostream& operator<<(ostream& ostr, const person& p) {
		ostr << p.name;
		return ostr;
	}
};

class employee : public person {
	//protected:
	int salary;

public:
	employee(int hw, const char* name) : person(name) {
		salary = hw;
	}

};

int foo(int a) {
	static int i = 5;
	i++;
	LOG(i);
	return 1;
}

void alien(char* c) {
	LOG(c);
}

void bar(const char* c) {
	alien(const_cast<char*>(c));
}

template<typename T>
T const& add(T t1, T t2) {
	return T(t1 + t2);
}

template <typename T>
class stack {
	vector<T> pStack;
public:

	T pop();
	void push(T elem);
};

template<typename T>
T stack<T>::pop()
{
	if (pStack.empty()) {
		throw std::out_of_range("Stack is empty. Cannot pop.");
	}
	T ret_val = pStack.back();
	pStack.pop_back();
	return ret_val;
}

template<typename T>
void stack<T>::push(T elem)
{
	pStack.push_back(elem);
}

class Point
{
public: 
	int x, y;
	Point(int i, int j) : x(i), y(j) { } // x gets value of i, y gets value of j, body is empty
};

class Rectangle12
{
public: 
	Point p1, p2;
	Rectangle12(int x1, int y1, int x2, int y2) : p1(x1, y1), p2(x2, y2) { }
};


int main()
{
	//Rectangle12 _rect(1, 2, 3, 4);
	cout << "tere";
	return 1;
}
	/*stack<int>_stack;
	_stack.push(1);
	_stack.push(2);
	_stack.push(3);

	try
	{
		LOG(_stack.pop());
		LOG(_stack.pop());
		LOG(_stack.pop());
		LOG(_stack.pop());
	}
	catch (exception& e) {
		LOG(e.what());
	}*/
	/*string b;
	auto a = add<double>(2, 3);
	LOG(typeid(a).name());
	LOG(typeid(add("re","aa")).name());
	cout << a;*/
	
//	return 0;
//
//	//return 1;
//
//	HMODULE hDLL = LoadLibraryA("C:\\Users\\S11M\\source\\repos\\powerplant\\powerplant\\IAS0410PlantEmulator.dll");
//	if (hDLL == NULL)
//	{
//		cout << "DLLExample.dll not found, error " << GetLastError() << endl;
//		return -1;
//	}
//
//	cout << "DLL loaded success" << endl;
//
//	FARPROC ptr1 = GetProcAddress(hDLL, "RunIAS0410PlantEmulator");
//	FARPROC ptr2 = GetProcAddress(hDLL, "SetIAS0410PlantEmulator");
//
//
//	if (ptr1 == NULL) {
//		cout << "RunIAS0410PlantEmulator() not found" << GetLastError() << endl;
//		FreeLibrary(hDLL);
//	}
//
//	else if (ptr2 == NULL) {
//		cout << "SetIAS0410PlantEmulator() not found" << GetLastError() << endl;
//		FreeLibrary(hDLL);
//	}
//
//	void (*RunPlant)(ControlData*) = reinterpret_cast<void (*)(ControlData*)> (ptr1);
//	void (*SetPlant)(string, int) = reinterpret_cast<void (*)(string, int)> (ptr2);
//	ControlData cData;
//
//
//	if (SetPlant != nullptr) {
//		try {
//			SetPlant("F:\\Downloads\\IAS0410 Instructors Stuff\\IAS0410Plants.txt", 6);
//		}
//		catch (exception& e) {
//			cerr << e.what() << endl;
//		}
//	}
//
//	if (RunPlant != nullptr) {
//		try {
//			cout << "Runninng plant";
//			RunPlant(&cData);
//		}
//		catch (const out_of_range& e) {
//
//		}
//		catch (exception& e) {
//			cerr << "Exception" << e.what() << endl;
//		}
//	}
//
//
//	FreeLibrary(hDLL);
//
//	return 1;
//}




// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
