# Design summary
## Threaded Alice → Internet → Bob Simulation
### Layout
```
internet_sim/
    ├── include/
        ├── Alice.hpp
        ├── Bob.hpp
        ├── Computer.hpp
        ├── Internet.hpp
        ├── Packet.hpp
        ├── Logger.hpp
        ├── PeriodicProcess.hpp
    ├── src/
        ├── Alice.cpp
        ├── Bob.cpp
        ├── Computer.cpp
        ├── Internet.cpp
        ├── Packet.cpp
        ├── Logger.cpp
        ├── PeriodicProcess.cpp
```
### Design Overview

- **PeriodicProcess** is an abstract base class that owns a thread and calls the virtual step() once per period.
- **Alice**, **Internet**, and **Bob** derive from **PeriodicProcess** and implement only step().
- **Computer** is the shared resource. Its outbox and inbox are each protected by their own mutex, and "take all" operations are atomic so no packet is lost or duplicated.
- **Logger** serializes console output so lines from different threads never interleave.
- **Packet** is a small data-only class with an id(), source(), dest(), and body().



