# Assignment  : DSA Queue Simulator  
**Name:** Siddhartha Karmacharya  
**Roll Number:** 35  

---

## 1. Summary of Work

This project implements a **real-time traffic intersection simulator** using **SDL3** for graphical visualization and **Windows Named Pipes** for inter-process communication.

The system consists of two main components:

1. **Traffic Simulator (`simulator.c`)**  
   - Visually simulates a four-way road intersection.
   - Displays roads, lanes, traffic lights, and moving vehicles.
   - Controls traffic lights dynamically based on traffic density.
   - Uses multithreading to handle traffic logic and vehicle input concurrently.

2. **Traffic Generator (`traffic_generator.c`)**  
   - Simulates incoming vehicles by generating random vehicle IDs.
   - Sends vehicle data (incoming and outgoing direction) to the simulator through a named pipe.
   - Acts as a separate process to mimic real-world traffic input.

The simulator prioritizes roads with higher congestion and adjusts green light durations dynamically to improve traffic flow.

---

## 2. Data Structures Used

| Data Structure | Implementation | Purpose |
|---------------|---------------|---------|
| `Vehicle` | Structure containing vehicle ID and progress | Represents an individual vehicle and its position |
| `Queue` | Array-based queue with `front` and `rear` indices | Stores vehicles waiting on each lane |
| `SharedData` | Structure containing traffic queues and light state | Shared state between threads for traffic control |
| Named Pipe Buffer | Character array | Transfers vehicle data between processes |

---

## 3. Functions Using Data Structures

### Queue Operations
- `initQueue(Queue* q)`
- `enqueue(Queue* q, char value[])`
- `dequeue(Queue* q)`
- `isEmpty(Queue* q)`
- `isFull(Queue* q)`
- `queueSize(Queue* q)`

### Traffic Processing
- `drawVehicles(SDL_Renderer*, SharedData*)`
- `chequeQueue(void* arg)`
- `pipeListenerThread(void* arg)`

### Traffic Light Control
- `refreshLight(SDL_Renderer*, SharedData*)`
- `drawLightForA/B/C/D(...)`

---

## 4. Algorithm Used for Traffic Processing

1. Vehicles are generated randomly and sent to the simulator using a **named pipe**.
2. Incoming vehicle data is parsed to determine:
   - Entry road
   - Exit direction
   - Lane type (left, straight, right)
3. Vehicles are placed into corresponding lane queues.
4. The traffic control thread periodically:
   - Counts vehicles in straight and right lanes.
   - Calculates average congestion.
   - Determines green light duration based on queue size.
   - Assigns priority to the most congested road.
5. During rendering:
   - Vehicles move forward based on light state.
   - Vehicles stop at red lights and proceed during green.
   - Vehicles are dequeued once they cross the intersection.

---

## 5. Time Complexity Analysis

Let:
- **R = 4 roads**
- **L = 3 lanes per road**
- **N = number of vehicles per lane**

### Queue Operations
- `enqueue()` → **O(1)**
- `dequeue()` → **O(1)**
- `queueSize()` → **O(1)**

### Traffic Congestion Check
- Iterates through all roads and lanes:
  - **O(R × L) = O(1)** (constant, fixed size)

### Vehicle Rendering and Movement
- Iterates through all vehicles:
  - **O(N)** per frame

### Overall Complexity
- **Per frame:** `O(N)`
- **Traffic control logic:** `O(1)`

Since the number of roads and lanes is fixed, the algorithm scales linearly with the number of vehicles.

---

## 6. Source Code Link

🔗 **GitHub Repository:**  
<https://github.com/your-username/your-repository-name>

---

## 7. Technologies Used

- **Language:** C
- **Graphics:** SDL3
- **Concurrency:** Windows Threads
- **IPC:** Windows Named Pipes
- **Platform:** Windows

---

## 8. How to Run

1. Compile and run `simulator.c`
2. In a separate terminal, compile and run `traffic_generator.c`
3. Vehicles will start appearing and traffic lights will adapt automatically.

---

## 9. Conclusion

This project demonstrates the use of **data structures**, **multithreading**, **IPC**, and **real-time rendering** to simulate an intelligent traffic control system. The design allows dynamic adaptation to traffic congestion, making it closer to real-world traffic management systems.

