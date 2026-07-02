# FlightWall Layout

### UI Mockups
**Commercial**
![Commercial](assets/commercial.png)
**Non-Commercial**
![Non-Commercial](assets/noncommercial.png) 


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

Maintenance (Thread 4):
    Every 30 seconds:
    - Update planes with result queue
    - Clean stale planes
    - Select plane
    - Write to snapshot

Snapshot

Renderer (Thread 5)
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

Aircraft Stuct:
``` C++
struct Aircraft {
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
    bool has_logo;
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
    // Method to enrich from enrich json
};
```

Main Psudeo Code:
``` psuedo
lastUpdate
while true:
    msg = parse(messageQueue.pop())
    aircraft = aircrafts.find(msg.iaco)

    if (!aircraft)
        new = Aircraft(msg)
        aircraft.update(msg)
        aircrafts.add(new)
        enrichQueue.push([new.iaco, new.callsign])
        continue

    aircraft.update(msg)
```

## 3: Maintenance

Psuedo Code:
``` psuedo
    if time.now - lastUpdate >= 30:

        // Update planes with result queue
        for response in responses:
            aircraft = aircrafts.find(response.iaco)
            aircraft.enrich(response)
            
        // Clean stale planes
        for aircraft in aircrafts:
            if time.now - aircraft.lastSeen >= 60:
                aircrafts.remove(aircraft.iaco)
    
        feature = select(aircrafts)
        snapshot.write(feature)
```

## 4: Enriching

Source: [adsbd.com]("https://www.adsbdb.com/")

API Call: `https://api.adsbdb.com/v0/aircraft/{ MODE_S || REGISTRATION }?callsign={ CALLSIGN_ICAO || CALLSIGN_ICAO }`
- MODE_S = icao
- CALLSIGN_ICAO = callsign

Response (Relevant fields only):
```
{
    "response": {
        "aircraft": {
            "manufacturer": "Cirrus", // Needed for non-commercial UI only
        },
        "flightroute": {
            "callsign_iata": "FR1054",
            "airline": {
                "name": "Ryanair"
            },

            // Might use lat/lon for path UI
            "origin": { 
                "iata_code": "EDI",
                "latitude": 55.950145, 
                "longitude": -3.372288,
            },
            "destination": {
                "iata_code": "PRG",
                "latitude": 50.1008,
                "longitude": 14.26,
            }
        }
    }
}
```

Psuedo Code:
``` psuedo
base = "https://api.adsbdb.com/v0/aircraft/"
while true:
    req = enrichQueue.pop()
    url = base + req(0) + "?callsign=" + req(1)
    response = get(url)
    json = parse(response.text)
    // Add logic to retrieve logo
    resultQueue.push(json)
```

## 5: Renderer

High Level:

I created the concept of an element as another layer of abstraction on top of text. The reason was to be able to apply fiting rules to associated text ( In fit mode A telemetry value cannot be draw without its unit). These fitting rules are apply at the caller level.

### Model to UI:

``` c++

const rgb_matrix::Color RED(227, 36, 0);
const rgb_matrix::Color LIGHT_RED(255, 140, 130);
const rgb_matrix::Color ORANGE(217, 80, 0);
const rgb_matrix::Color YELLOW(255, 251, 185);
const rgb_matrix::Color LIGHT_YELLOW(254, 252, 221);
const rgb_matrix::Color GREEN(119, 187, 65);
const rgb_matrix::Color BLUE(116, 167, 254);
const rgb_matrix::Color LIGHT_BLUE(212, 227, 254);

const rgb_matrix::Color WHITE(235, 235, 235);
const rgb_matrix::Color GREY(214, 214, 214);
const rgb_matrix::Color BEIGE(255, 242, 213);


struct Row {
    std::vector<Element> elements;
    int space;
    Mode mode;
}

struct Element {
    vector<Text> items;
    int space;
    Mode mode;
};

struct Text {
    const std::string value;
    const Font& font;
    const Color& color;
}

enum class Mode {
    clip,
    fit,
    scroll
};

struct AircraftDisplay {
    magick::image logo;
    std::vector<Row> rows;
    Rect content;

    AircraftDisplay(const Aircraft& a, const Theme& t,
                    const Font sml, const Font med, const Font lrg) {
        
        switch (t) {
            case Theme::Com:
                rows.push_back(Row{{Element{Mode::Scroll, 0, {Text{a.airline, lrg, RED}}}}, 0});
                rows.push_back(Row{{Element{Mode::Fit, 0, {Text{a.flt_no, sml, LIGHT_RED}}},
                                    Element{Mode::Fit, 0, {Text{"·", sml, GREY},
                                                              Text{a.typ_code, sml, LIGHT_RED}}}}, 2});
                break;

            case Theme::Non:
                rows.push_back(Row{{Element{Mode::Scroll, 0, {Text{a.callsign, lrg, RED}}}}, 0});
                rows.push_back(Row{{Element{Mode::Fit, 0, {Text{a.flt_no, sml, LIGHT_RED}}}}, 0});
                break;
        }

        rows.push_back(Row{{Element{Mode::Fit, 2, {Text{a.speed, sml, BLUE},
                                                      Text{"mph", sml, LIGHT_BLUE}}},
                            Element{Mode::Fit, 2, {Text{a.alt, sml, BLUE},
                                                      Text{"ft", sml, LIGHT_BLUE}}}}, 2});
        
        switch (t) {
            case Theme::Com:
                rows.push_back(Row{{Element{Mode::Fit, 0, {Text{a.orgn, med, LIGHT_RED}}},
                                    Element{Mode::Fit, 0, {Text{"→", med, BEIGE}}},
                                    Element{Mode::Fit, 0, {Text{a.dest, med, GREEN}}}}, 2});
                break;
            case Theme::Non:
                rows.push_back(Row{{Element{Mode::Fit, 2, {Text{a.dist, med, YELLOW},
                                                              Text{"mi", sml, LIGHT_YELLOW}}},
                                    Element{Mode::Fit, 0, {Text{a.brg, med, YELLOW}}},
                                    Element{Mode::Fit, 2, {Text{"-", med, YELLOW},
                                                              Text{a.trk, med, YELLOW},
                                                              Text{"°", sml, LIGHT_YELLOW}}}}, 2});
                break;
        }
    }
}

struct Position {
    Text text;
    int x;
    int y;
    int l;
    int r;
}

std::vector<positions> layout(const AircraftDisplay& disp, int64_t time){
    const int y1 = disp.logo.rows(); // Anchor to bottom line of logo
    const int y2 = y1 + disp.rows[2].h + disp.row_gap;
    const int y4 = disp.content.btm(); // Anchor to bottom of frame
    const int y3 = y4 + disp.rows[4].h + disp.row_gap; 

    int x = disp.frame.left();
    const int x1 = (1 <= disp.logo_span) ? x + img : x;
    const int x2 = (2 <= disp.logo_span) ? x + img : x;
    const int x3 = (3 <= disp.logo_span) ? x + img : x;
    const int x4 = (4 <= disp.logo_span) ? x + img : x;

    vector<positions> pos;
    pos.push_back(lay_row(disp.rows[1], x1, y1));
    pos.push_back(lay_row(disp.rows[2], x2, y2));
    pos.push_back(lay_row(disp.rows[3], x3, y3));
    pos.push_back(lay_row(disp.rows[4], x4, y4));

    return pos;
}

std::vector<positions> lay_row(int x, int y, int r,
                               const Row& row, 
                               std::optional<int64_t> time){
    std::vector<Positions> pos;
    int start = x;

    for (size_t i = 0; i < row.elements.size(); i++){
        int w;=
        elmnt = row.elements[i];
        std::vector<Postion> np = lay_elmnt(x, y, &w, row.gap, elmnt);

        if (x + w  > r) {
            switch (row.mode) {
                case Mode::Fit:
                    return pos;
                
                case Mode::clip;
                    pos.push_back(np);
                    return pos;

                case Mode::scroll;
                    std::vector<Element> rest(row.elements.begin() + i, row.elements.end());
                    pos.push_back(np);
                    x = scroll_placement(start, x, Row{rest, row.gap, row.mode});
                    pos.push_back(lay_elmnt(x, y, &w, row.gap, elmnt));
                    x += w;
                    continue;
            }
        }

        pos.push_back(np);
        x += w;
    }
}

int scrl_plcmnt(int strt, int l, int r, 
                int scrl_gap, int pps,
                int64_t time, 
                const Row& rest) {
    
    int scrl_ofst = time * (pps / 1000) % (r - l)
    strt += scrl_ofst;
    int w = msr_row(rest);
    int r = start + w - r;
    int offset = w - leftover;
    int max_x = start - w - scrl_gap;

    if ( l < max_x)
        return l - offset;
    
    return max_x - offset;
}

int msr_row(const Row& row) {
    int w = 0;
    for (size_t i = 0; i < row.elements.size(); i++){
        Element elmnt = rest[i];
        w += msr_elmnt(elmnt, row.gap);

        if (i < row.elements.size() - 1)
            w += row.gap;
    }
    return w;
}

int msr_elmnt(Element elmnt){
    int w = 0;
    for (size_t i = 0; i < elmnt.items.size(); i++>) {
        Text itm = elmnt.items[i]
        w += MeasureText(itm.value, itm.font);

        if (i < elmnt.items.size() - 1)
            w += elmnt.gap;
    }
    return w;
}
// TODO: make robust and type, qualifier decsions


std::vector<Position> lay_elmnt(int x, int y, int& w,  int gap,
                                 Element& elmnt) {
    int start = x;
    std::vector<Position> pos
    for (size_t i = 0; i < elmnt.items.size(); i++) {
        Text itm = elmnt.items[i];
        Position np{itm, x, y};
    
        x += MeasureText(itm.value, itm.font);
        if (i < elmnt.items.size() - 1)
            x += gap;

        pos.push_back(np);
    }
    w = x - start;
}

```

### UI Layer:
Classes:
``` c++
// Likely needed, highly intuitive
class Rect {
    public:
        int x;
        int y;
        int h;
        int w;

        Rect(int x, int y, int h, int w):
            x(x), y(y), h(h), w(w) {}

        void inset(int padding){
            x += padding;
            y += padding;
            h -= padding * 2;
            w -= padding * 2;
        }

        int lft() { return x; }
        int rght() { return x + w; }
        int tp() { return y; }
        int btm() { return y - h; }
};

void draw(const std::vector<Placement>& positions){
    for (const auto& pos : positions)
        DrawText(pos.x, pos.y, pos.l, pos.r, 
                 pos.text.val, pos.text.font,
                 pos.text.c);
}   
```

``` c++
// From image-example.cc
void draw_image(Canvas *c, int x, int y, const Magick::Image &image) {
    for (size_t y = 0; y < image.rows(); ++y) {
    for (size_t x = 0; x < image.columns(); ++x) {
      const Magick::Color &c = image.pixelColor(x, y);
      if (c.alphaQuantum() < 256) {
        canvas->SetPixel(x + offset_x, y + offset_y,
                         ScaleQuantumToChar(c.redQuantum()),
                         ScaleQuantumToChar(c.greenQuantum()),
                         ScaleQuantumToChar(c.blueQuantum()));
      }
    }
  }
}

static Magick::Image load_image(const std::string& path){
    Magick::Image image(path);
    image.scale(Magick::Geometry("x14")); // Scale height to 14px, auto width
    return image;
}
```

Hzeller Library Edit:
``` c++
// In bdf-font.cc

// Skip drawing pixels outside of clip bounds
int Font::DrawGlyph(Canvas *c, int x_pos, int y_pos,
                    const Color &color, const Color *bgcolor,
                    uint32_t unicode_codepoint,
                    int l_clip, int r_clip) const {
  const Glyph *g = FindGlyph(unicode_codepoint);
  if (g == NULL) g = FindGlyph(kUnicodeReplacementCodepoint);
  if (g == NULL) return 0;
  y_pos = y_pos - g->height - g->y_offset;

  if (x_pos + g->device_width < 0 || x_pos > c->width() ||
      y_pos + g->height < 0 || y_pos > c->height() ||
      x_pos + g->device_width <= l_clip || x_pos >= r_clip) {
    return g->device_width;
  }

  for (int y = 0; y < g->height; ++y) {
    const rowbitmap_t& row = g->bitmap[y];
    for (int x = 0; x < g->device_width; ++x) {
      const int sx = x_pos + x;
      if (sx < l_clip || sx >= r_clip)
        continue;
      if (row.test(kMaxFontWidth - 1 - x)) {
        c->SetPixel(sx, y_pos + y, color.r, color.g, color.b);
      } else if (bgcolor) {
        c->SetPixel(sx, y_pos + y, bgcolor->r, bgcolor->g, bgcolor->b);
      }
    }
  }
  return g->device_width;
}

// Pass down clip parameters
int DrawText(Canvas *c, const Font &font,
             int x, int y, const Color &color, const Color *background_color,
             const char *utf8_text, int extra_spacing,
             int l_clip = INT_MIN, int r_clip = INT_MAX) {
  const int start_x = x;
  while (*utf8_text) {
    const uint32_t cp = utf8_next_codepoint(utf8_text);
    x += font.DrawGlyph(c, x, y, color, background_color, cp, l_clip, r_clip);
    x += extra_spacing;
  }
  return x - start_x;
}
```

Main:
``` Psuedo
// setup canvas

last;
while true:
    plane = snapshot.read()

    if (last.iaco == plane.iaco) {

    }
```



