# TalTech IAS0410 Object Oriented Programming - PlantLogger project
This repository contains code for the IAS0410 Object Oriented Programming Master's course Project called PlantLogger.

The complete specification can be found on this website: https://www.tud.ttu.ee/im/Viktor.Leppikson/IAS0410%20Practical%20work%202023.pdf

The repository includes complete working and graded code for:
* Plant 6
* Datastructure 4

### Short overview:
* Multithreaded application
* Launches Plant data producer thread
* Launches Plant data consumer thread
* Controller is runnning on main thread
* Producer - Writes byte formatted data packages to a shared buffer, see [3. Measurement results](https://www.tud.ttu.ee/im/Viktor.Leppikson/IAS0410%20Practical%20work%202023.pdf)
* Consumer - Parses the data, writes to binary file and also Data4 structure (scroll down for reference).
* Controller - Controls the working of the "Plant" via keyboard input, see [5. Keyboard commands controlling the logger](https://www.tud.ttu.ee/im/Viktor.Leppikson/IAS0410%20Practical%20work%202023.pdf)
* The measurments are written into conole and also saved into a binary file

#### Shared data between producer and consumer thread
```
struct ControlData 
{
  mutex mx;
  condition_variable cv;
  atomic<char> state;
  vector<unsigned char> *pBuf;
  promise<void> *pProm;
};
```
#### Data structure where the measurments are saved into

It is a C++ map in which the channel names are the keys.<br>The values are
pointers to inner C++ maps in which the keys are point names and the
values are pointers to lists.<br>The members of lists are pairs in which
member "first" is the measument value and member "second" is the
timestamp.
```
map<string, map<string, list<pair<variant<int, double>, system_clock::time_point> > * > * > Data4;
```
