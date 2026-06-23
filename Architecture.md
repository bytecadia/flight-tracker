FlightWall Layout

![Alt text](assets/commercial.png) 
![Alt text](assets/noncommercial.png) 



## Intro 1: Thread safety

### Thread Safe Snapshot


``` C++
class Snapshot {
private:
    Aircraft _a;
    mutable std::mutex _mtx;

public:

    void write(const Aircraft a){
        std::lock_guard<std::mutex> lock(_mtx);
        _a = a;
    }

    Aircraft read(){
        std::lock_guard<std::mutex> lock(_mtx);
        return _a;
    }
};
```


### Thread Safe Queue

High Level:
Class implements the standard queue but adds mutex and condition variables. Each operation (pop or push) will aquire a mutex lock then operated on queue after scope ends mutex is unlocked. 

Implementation:

Check here for more detailed implementation:
[Reference](https://medium.com/@abhishek.kr121/thread-safe-queue-implementation-c3d63c1c6d7f)

Includes:

``` c++
#include <queue>
#include <mutex>
#include <condition_variable>
```

TODO: Needs way to shutdown
``` c++
template <typename T> // Class works with any type
class TSQueue {
private:
    std::queue<T> _q; // queue for any element
    mutable std::mutex _mtx; // lock access to queue / Why mutable? -> because idk i forgo im toired
    std::condition_variable _cv; // Signal to other thread

public:
    TSQueue(const TSQueue&) = delete; // Remove copy costructor
    TSQueue& operator=(const TSQueue&) = delete; // Remove copy assignment operator


    void push(const T& val) { // pass reference to not copy but make const so this function can't change shi
        {
            std::lock_guard<std::mutex> lock(_mtx); // lock mutex (lock guard releases at end of scope)
            _q.push(val);
        } // end mutex scope
        _cv.notify_one(); // Notiy one of waiting thread 
    }

    T pop(){
        std::unique_lock<std::mutex> lock(_mtx); // unique lock for pop b/c wait needs to unlock and relock the mutex while thread sleeps
        while (_q.empty()){
            _cv.wait(lock);
        }
        T t = std::move(_q.front()); // Retrieve first in line then move ownership
        _q.pop();
        return t;
    }
    
    // empty()
    // size()
}
```



## Intro 2: Makefiles


## Intro 3: Proposed architecture

```
Socket Reader (Thread 1)
    - Aquires mutex
    - Pushes to Queue
    - Notifies other thread
    - Stale Handle: When queue is full remove oldest

Raw message Queue 

Main (Thread 2).                            -->  Enrich Queue  -->  Enrich (Thread 3)
    On every message (q.pop()):                                       - pop from queue
    - pop message from queue.                                         - call api
    - parse msg, update plane               <--  Result Queue         - enrich aircraft object               
    - if new plane push to enrich queue                               - push to queue

    On every 30 sec:
    - Update planes with result queue
    - Clean stale planes
    - Select plane
    - Write to snapshot

Snapshot

Renderer (Thread 4)
    - Builds canvas
    - Draws to matrix

```

## 1: Read ADSB Messages from Socket

Input: Byte streams
Output: std::vector<std::string> of individual ADSB messages

High Level:

Client socket connection to dump1090 server
- Use POSIX getaddrinfo() to retrieve identity of host and service
- Look up is done using host name, port number, address type, and address family
- Init addrinfo struct and pass address to hold returned linked list
- Call socket() sys call socket object with family (internet), type (stream), and protocol (TCP)
    - Here we just use the fields pulled by getaddrinfo in addrinfo struct
    - We use internet becasue dump1090 sends TCP data over socket at port 3003 
- Call conect() sys call using the socketfd, address, address len

Buffering byte stream and dividing bytes into messages
- Use std string (to use .substring method) to hold buffer
- User char[] for chunk at standard 4096 size
- Start outer loop to read bytes into chunck from socket using recv and append chunk to buffer
- Break out if ssize_t byte number is negative (connection lost or error)
- inner loop to seperate buffer by delimiter (using find to find position and substr to save new message)
- erase from buffer 0 to delimter
- handle new message (add to queue)

Thread safety:

Producer Thread -> Queue --> Consumer Thread

Implementation:

Includes:

<cstdio> : perror, etc.
<cstdlib> : exit
<string> / <vector> : own and return the parsed messages
<sys/types.h> : data types for sys calls
<sys/socket.h> : structures needed for sockets
<netdb.h> : getaddrinfo + addrinfo 

Structs:

``` C++
struct addrinfo // resolver fills a linked list of these; protocol-agnostic (IPv4/IPv6)
{
  int              ai_flags;     // AI_PASSIVE, AI_CANONNAME, ...
  int              ai_family;    // AF_INET, AF_INET6, AF_UNSPEC
  int              ai_socktype;  // SOCK_STREAM for the ADSB TCP feed
  int              ai_protocol;  // 0 = any
  socklen_t        ai_addrlen;   // length of ai_addr
  struct sockaddr *ai_addr;      // ready-to-use binary address (pass straight to connect, old version had to convert yourself)
  char            *ai_canonname; // canonical host name
  struct addrinfo *ai_next;      // next candidate address, or nullptr
};
```

Helpers:

``` C++
int socket_connection()
{
    const char *host = "localhost";
    const char *port = "30003";       // ADSB feed port, as a service string for getaddrinfo

    // Hints to resolver what kind of address is needed
    struct addrinfo hints{};         // value-init to all zero
    hints.ai_family   = AF_UNSPEC;   // IPv4 or IPv6
    hints.ai_socktype = SOCK_STREAM; // TCP

    struct addrinfo *res = nullptr;  // resolver outputs linked list (free with freeaddrinfo)
    if (getaddrinfo(host, port, &hints, &res) != 0) // 0 if success
        error("getaddrinfo");

    int sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol); // AF_INET or AF_INET6 (Internet), STREAM, TCP

    if (sockfd < 0) // Error check
        error("socket");

    // What about other members of linked list?
    if (connect(sockfd, res->ai_addr, res->ai_addrlen) < 0) // fd, binary address, length
        error("connect");

    freeaddrinfo(res); // Free linked list

    // accumulate below
    // close fd - could use RAII wrapper (closes on scope exit)
}
```

``` C++
std::string buffer;
char chunk[4096]; // common chunk size

// Outer loop to read chunk message and add to buffer
while (true)
{
    ssize_t n = recv(sockfd, chunk, sizeof(chunk), 0); // receive bytes from socket and save into chuck with default behavior (flags = 0)

    if (n <= 0)
        break; // disconnect or error

    buffer.append(chunk, n); // Add chunk of message to buffer

    size_t pos;
    // Inner loop to divide buffer into messages using delimeter
    while ((pos = buffer.find("\r\n")) != std::string::npos) // npos means not found
    {
        std::string msg = buffer.substr(0, pos); // Save message

        buffer.erase(0, pos + 2); // 2 for both \r and \n
        // Handle msg
    }
}
```

## 2: Main Aircraft Processing

Classes:

``` C++
class Aircraft {
    public:
    enum class Operation {
        com,
        non,
        ukn
    }
    enum class Kind {
        prop,
        jet,
        heli,
    }

    // SBS Data

    // Required
    const std::string icao;


    // Optional
    std::string callsign;
    int track;
    double lat;
    double lon;
    int altitude;
    int groundSpeed;
    bool isOnGround;

    // API Data - Optional
    const std::string airline;
    const std::string flightNo;
    const std::string typeCode;
    Operation op;
    Kind kind;

    // Derived Data - Optional
    double distance;
    //[time type] lastSeen;

    // Constructor that takes in just the IACO and defaults the other fields

    // Method to update fields from on msg string
    // Method to determine distance based on location
};

```

``` c++
class AircraftRegistry {
  public:
    std::unordered_map<std::string, Aircraft> aircrafts;


    // Method to look up by IACO
    // Method to select aircraft
    // Method to add/remove aircraft
    // Method to remove stale aircraft
}
```
Main while loop:
    pop message from queue
    update plane
    if new plane push to enrich queue 


    On every 30 sec:
      - Update planes with result queue
      - Clean stale planes
      - Select plane
      - Write to snapshot
      
Enrich:

API Call: `https://api.adsbdb.com/v0/aircraft/{ MODE_S || REGISTRATION }?callsign={ CALLSIGN_ICAO || CALLSIGN_ICAO }`
- MODE_S = icao
- CALLSIGN_ICAO = callsign

Response:
```
{
    "response": {
        "aircraft": {
            "manufacturer": "Cirrus", // Needed for non-commercial UI only
        },
        "flightroute": {
            "callsign": "RYR1054",
            "callsign_icao": "RYR1054",
            "callsign_iata": "FR1054",
            "airline": {
                "name": "Ryanair",
                "icao": "RYR",
                "iata": "FR",
                "country": "Ireland",
                "country_iso": "IE",
                "callsign": "RYANAIR"
            },
            "origin": {
                "country_iso_name": "GB",
                "country_name": "United Kingdom",
                "elevation": 135,
                "iata_code": "EDI",
                "icao_code": "EGPH",
                "latitude": 55.950145,
                "longitude": -3.372288,
                "municipality": "Edinburgh",
                "name": "Edinburgh Airport"
            },
            "destination": {
                "country_iso_name": "CZ",
                "country_name": "Czech Republic",
                "elevation": 1247,
                "iata_code": "PRG",
                "icao_code": "LKPR",
                "latitude": 50.1008,
                "longitude": 14.26,
                "municipality": "Prague",
                "name": "Václav Havel Airport Prague"
            }
        }
    }
}
```


