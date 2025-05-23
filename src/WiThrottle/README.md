# WiThrottle library

This library implements the WiThrottle protocol (as used in JMRI and other servers), allowing an device to connect to the server and act as a client (such as a dedicated fast clock device or a hardware based throttle).

The implementation of this library is tested on ESP32 based devices running the Arduino framework.   There's nothing in here that's specific to the ESP32, and little of Arduino that couldn't be replaced as needed.

## Basic Design Principles

First of all, this library implements WiThrottle in a non-blocking fashion.  After creating a WiThrottle object, you set up various necessities (like the network connection and a debug console) (see [Dependency Injection][depinj]).   Then call the ```check()``` method as often as you can (ideally, once per invocation of the ```loop()``` method) and the library will manage the I/O stream, reading in command and calling methods on the [delegate] as information is available.

These patterns (Dependency Injection and Delegation) allow you to keep the different parts of your sketch from becoming too intertwined with each other.  Nothing in the code that manages the pushbuttons or speed knobs needs to have any detailed knowledge of the WiThrottle network protocol.

## Differences from the original library

 - Removed dependencies with external libraries (Chrono.h, ArduinoTime.h, TimeLib.h) 
 - Added NullStream class to disable (by default) logging
 - Changed begin() method to setLogStream()
 - Added a setter method for delegate class: setDelegate()
 - Added the ability to parse roster messages and to receive the roster list via delegate class

## Included examples

### WiThrottle_Basic

Basic example to implement a WiThrottle client and connect to a server (with static IP).

Change the WiFi settings and enter the IP address of the computer running JMRI (or other WiThrottle server):
```
const char* ssid = "MySSID";
const char* password =  "MyPWD";
IPAddress serverAddress(192,168,1,1);
```
Compile and run, you should see a new client connected in JMRI:

![](https://github.com/lucadentella/WiThrottle/raw/master/images/basic-example.jpg)

### WiThrottle_Delegate

Example to show how to implement a delegate class to handle callbacks.

Compile and run, you should see in Serial monitor the server version, printed by ```void receivedVersion(String version)``` method:

![](https://github.com/lucadentella/WiThrottle/raw/master/images/delegate-example.jpg)

### WiThrottle_FastTime

Example to show how to get the fasttime from WiThrottle server, and how to transform the timestamp in HOUR:MINUTE format. As explained above, I removed all the external dependences: the library returns a timestamp (method ```getCurrentFastTime()```) and you can choose your preferred library (or none) to parse it.

For this example you need the [```Time``` library](https://github.com/PaulStoffregen/Time), which can be installed through IDE Library Manager.

Compile and run. Use a proper terminal (MobaXterm in my screenshot) to see the time updated in the same line:

![](https://github.com/lucadentella/WiThrottle/raw/master/images/fastclock-example.jpg)

### WiThrottle_Roster

Example to show how to get the list of locomotives in the roster. The library parses the roster message (RLx) but **doesn't** store the list. Instead, it offers two callbacks to get the number of entries and, for each entry, name / address / length (**S**hort|**L**ong).

Compile and run, you should see in the Serial monitor the list of locomotives as defined in JMRI:

![](https://github.com/lucadentella/WiThrottle/raw/master/images/roster-example.jpg)

## Public Methods

### Basic Setup & Use
```
WiThrottle(bool isServer)
```
Create a new WiThrottle manager.   You probably only need one in your program, unless you connect to multiple WiThrottle servers at the same time.   

```
void begin(Stream *console)
```
Initializes the WiThrottle object.   You should call this as part of your ```setup``` function.  You must pass in a pointer to a ```Stream``` object, which is where all debug messages will go.  This can be ```Serial```, or anything similar.  If you pass in ```NULL```, no log messages will be generated.

```
void connect(Stream *network)
```
Once you have created the network client connection (say, via ```WiFiClient```), configure the WiThrottle library to use it.  After you've connected the client to WiThrottle, do NOT perform any I/O operations on the client object directly.   The WiThrottle library must control all further use of the connection (until you call ```disconnect()```).

```
void disconnect()
```
Detach the network client connection from the WiThrottle library.   The WiThrottle library will do very little of value after this is called.   I'm not entirely sure this is a useful method, but if I add a ```connect()``` I feel compelled to add ```disconnect()``` for completeness.    

```
bool check()
```
This drvies all of the work of the library.  Call this method at least once every time ```loop()``` is called.   When using the library, make sure that you do not call ```delay()``` or anything else which takes up significant amounts of time.   

If this method returns ```true```, then a command was processed this time through the loop.  There will be many, many, many more times that ```check()``` is called and it returns ```false``` than when it returns ```true```.   

```
WiThrottleDelegate *delegate
```
This variable holds a pointer to a class that subclasses the ```WiThrottleDelegate``` class.  Once this is set, various methods will be called when protocol commands come in and have been processed.

### Fast Time 
```
int fastTimeHours()
```
Returns the current fast time clock Hour value (0-23).


```
int fastTimeMinutes()
```
Return the current fast time clock Minute value (0-59).


```
float fastTimeRate()
```
Return the current fast time clock ratio.  If the value is 0.0, the clock is not running.  Otherwise, this is the number of seconds that should pass in fast time for every second of real time that elapses.

```
bool clockChanged;
```
This value will be set to ```true``` on each call of ```check()``` when the fast time is actually changed.  Otherwise the value will be set to ```false```.   Perhaps this (and the fastTime* methods) should all be transitioned to delegate methods.

```
void requireHeartbeat(bool needed)
```
Sends the ```*``` command with either ```+``` or ```-``` to indicate that heartbeat messages are needed (or not).

```
void addLocomotive(String address)
```
Select the given locomotive address.  The address must be in the form "Snnn" or "Lnnn", where the S or L represent a short or long address, and nnn is the numeric address.   Returns ```true``` if the address was properly formed, ```false``` otherwise.

Note that the WiThrottle library does not yet support adding multiple locomotives at the same time.  

```
bool stealLocomotive(String address)
```
Attempt to steal a locomotive address by releasing it and then selecting it.  Return ```true``` the address is properly formed, ```false``` otherwise.

```
bool releaseLocomotive(String address="*")
```
Release the specified locomotive address.  If the address is specified as "*" (or not explicitly given, as this is the default value), all locomotives will be released.

### Function State
```
void setFunction(int num, bool pressed)
```
Update the state for the specified function (0-28 is the acceptable range).  If the function button has been pressed down (or otherwise activated), set ```pressed``` to ```true```.   When the button is released, set ```pressed``` to ```false```. 

### Speed & Direction
```
bool setSpeed(int value)
```
Set the DCC speed value (0-126).  If there is no locomotive selected, or if the speed value is out of range, returns ```false```, otherwise this returns ```true```.

```
int getSpeed()
```
Return the current speed value.  This is not meaningful if no locomotive is selected.


```
bool setDirection(Direction d)
```
Set the direction of travel.  The values for ```d``` can be ```Forward``` or ```Reverse```.


```
Direction getDirection()
```
Returns the current direction of travel.  Will be ```Forward``` or ```Reverse```.

```
void emergencyStop()
```
Send an emergency stop command.


## Delegate Methods

These methods will be called if the ```delegate``` instance variable is set.   A class may implement any or all of these methods.  

```
void receivedVersion(String version)
```
The WiThrottle protocol version.  When the ```VNx.y``` command is received, this method will be called with the ```version``` parameter set to "x.y".

```
void fastTimeChanged(uint32_t time)
```
Called whenever the fast time changes.  ```time``` is a standard Unix time value.   This should probably be changed to provide hour & minute parameters instead of forcing the callee to parse unix time.  

```
void fastTimeRateChanged(double rate)
```
A new fast clock time ratio has been received.   

```
void heartbeatConfig(int seconds)
```
Called when the expected heartbeat time changes.  If the ```seconds``` value is non-zero, then the heartbeat command must be sent before that many seconds has passed.   Calling the ```check()``` method will ensure that the heartbeat command is sent on time.

```
void receivedFunctionState(uint8_t func, bool state)
```
Indicates that a function has changed state.  This is to be used when there is some feedback to the user (such as a light).  If ```state``` is ```true```, then the function is ON, otherwise the function is OFF.

```
void receivedSpeed(int speed)
```
Indicates that the current speed value has been changed to ```speed```.  This might be done when a JMRI throttle is also opened up for the selected address.   

```
void receivedDirection(Direction d)
```
Indicates that the current direction has been changed.  ```d``` will be ```Forward``` or ```Reverse```.

```
void receivedSpeedSteps(int steps)
```
Indicates that the selected locomotive is configured with the given number of speed steps.

```
void receivedWebPort(int port)
```
Indicates that the WiThrottle is running an auxiliary web server on the given TCP port (at the same address).


```
void receivedTrackPower(TrackPower state)
```
Indicates that the layout track power has changed.  The value will be ```PowerOff```, ```PowerOn``` or ```PowerUnknown```.


```
void addressAdded(String address, String rosterEntry)
```
Indicates that the given ```address``` has been added to the current throttle.  The ```rosterEntry``` value may be populated as well.

```
void addressRemoved(String address, String command)
```
Indicates that the given ```address``` has been removed from the current throttle.   ```command``` will be "d" (if dispatched) or "r" (if released).

```
void addressStealNeeded(String address, String entry)
```
Indicates that the ```address``` cannot be selected because it is already in use.  You may wish to prompt the user if they wish to steal this locomotive (which can be then be done by using the ```stealLocomotive``` method).



## Todos

 - Write Tests
 - More complete WiThrottle protocol parsing
 - Better Parser (Antlr?)
 

License
----

Creative Commons [CC-BY-SA 4.0][CCBYSA]   ![CCBYSA](https://i.creativecommons.org/l/by-sa/4.0/88x31.png)


**Free Software, Oh Yeah!**

[//]: # (These are reference links used in the body of this note and get stripped out when the markdown processor does its job. There is no need to format nicely because it shouldn't be seen. Thanks SO - http://stackoverflow.com/questions/4823468/store-comments-in-markdown-syntax)

   [depinj]: <https://en.wikipedia.org/wiki/Dependency_injection>
   [delegate]: <https://en.wikipedia.org/wiki/Delegation_(object-oriented_programming)>
   [CCBYSA]: <http://creativecommons.org/licenses/by-sa/4.0/>
   
