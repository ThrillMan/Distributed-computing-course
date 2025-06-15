# Projektanci Światów - Distributed Computing Project

## 📖 Project Description

In a futuristic scenario where advanced technology is indistinguishable from magic, bored post-humans create worlds for entertainment. To accomplish this, they must form pairs of **geoengineers** and **artists**. These pairs then compete for X slots in an energy generator to begin world creation.

### Polish Description
> W przyszłości urzeczywistniła się znana maksyma "wystarczająco zaawansowana technologia jest nieodróżnialna od magii". Postęp nauki sprawił, że znudzeni post-ludzie mogą dla zabawy tworzyć światy. W tym celu muszą się dobrać w pary geoinżynierka-artysta. Następnie para ubiega się o X slotów w generatorze energii, po czym zaczynają tworzyć świat.

## 🎯 Problem Specification

- **Processes**: G geoengineers and A artists
- **Resource**: X indistinguishable energy generator slots
- **Requirements**:
  - Minimize the number of rounds in which processes pair up
  - Prevent process starvation
  - Implement heuristics to reduce the probability of identical pair formations
  - Pairing must result from communication between processes

## 🏗️ Implementation Details

### Architecture
The solution uses **MPI (Message Passing Interface)** to implement a distributed algorithm with the following components:

### Process Types
1. **Artists (A)** - Processes 0 to A-1
2. **Geoengineers (G)** - Processes A to total_size-1

### Communication Protocol

#### Message Tags
```c
#define REQUEST_TAG 1      // Request for critical section access
#define OK_TAG 2          // Permission granted for critical section
#define RELEASE_TAG 3     // Release critical section
#define PAR_REQUEST_TAG 4 // Pairing request
#define PAR_ACCEPT_TAG 5  // Pairing accepted
#define PAR_DENY_TAG 6    // Pairing denied
#define PAR_END_TAG 7     // End of pairing
```

#### Process States
```c
enum State { RELEASED, WANTED, HELD };
```

### Key Features

#### 1. Ricart-Agrawala Mutual Exclusion
- Artists use the Ricart-Agrawala algorithm to access the critical section
- Logical clocks ensure proper ordering of requests
- Deferred queue prevents starvation

#### 2. Anti-Starvation Heuristic
```c
int *pair_counts; // Track pairing frequency for each engineer
```
Artists maintain counters tracking how many times they've paired with each engineer, prioritizing those with fewer previous pairings.

#### 3. Distributed Pairing Protocol
- Artists in critical section send pairing requests to available engineers
- Engineers respond with acceptance or denial based on availability
- Successful pairs collaborate for a random duration

## 🔧 Algorithm Flow

### Artist Process Logic
1. **Request Phase**: Artist wants to enter critical section
   - Increment logical clock
   - Send REQUEST messages to all other artists
   - Wait for OK responses from all other artists

2. **Critical Section**: 
   - Try to pair with available engineers (prioritizing least-paired)
   - Send PAR_REQUEST to selected engineer
   - Handle PAR_ACCEPT/PAR_DENY responses

3. **Release Phase**:
   - Send OK responses to all deferred requests
   - Return to released state

### Engineer Process Logic
1. **Listen**: Wait for PAR_REQUEST messages
2. **Respond**: 
   - If available: send PAR_ACCEPT and enter paired state
   - If busy: send PAR_DENY
3. **Collaborate**: Work with artist until receiving PAR_END
4. **Reset**: Return to available state

## 🚀 Compilation and Execution

### Prerequisites
Installed MPI

### Compilation
```bash
mpicc -o world_designers main.c
```

### Execution
```bash
# Run with N processes (N should be even for equal A and G)
mpirun -np 6 ./world_designers

# Example with hostfile for cluster execution
mpirun -np 8 -hostfile hosts ./world_designers
```

## 📊 Performance Features

### Starvation Prevention
- **Logical clocks** ensure fair ordering of critical section requests
- **Deferred queues** guarantee all requests are eventually processed
- **Priority system** based on timestamp and process ID

### Efficiency Optimizations
- **Pair counting heuristic** reduces repeated identical pairings
- **Non-blocking communication** where possible
- **Distributed decision making** without central coordinator

## 🔍 Key Algorithms Implemented

### 1. Ricart-Agrawala Mutual Exclusion
```c
// Priority determination
if (p->req_clock < incoming_clock) {
    // Defer the request
    p->deferred_queue[p->deferred_count++] = status.MPI_SOURCE;
} else if ((p->req_clock == incoming_clock) && (p->rank < status.MPI_SOURCE)) {
    // Defer based on process ID
    p->deferred_queue[p->deferred_count++] = status.MPI_SOURCE;
} else {
    // Grant permission immediately
    MPI_Send(&p->clock, 1, MPI_INT, status.MPI_SOURCE, OK_TAG, MPI_COMM_WORLD);
}
```

### 2. Anti-Repetition Heuristic
```c
// Select engineer with minimum previous pairings
for (int g = a_num; g < p->size; g++) {
    if (p->pair_counts[g - a_num] == 0) {
        // Try to pair with this engineer
        MPI_Send(&p->rank, 1, MPI_INT, g, PAR_REQUEST_TAG, MPI_COMM_WORLD);
        // ... handle response
        if (successful_pairing) {
            p->pair_counts[g - a_num]++;
        }
    }
}
```

## 🧪 Testing

### Test Scenarios
1. **Small scale**: 4 processes (2 artists, 2 engineers)
2. **Medium scale**: 8 processes (4 artists, 4 engineers)
3. **Large scale**: 16+ processes
4. **Uneven loads**: Different work durations

### Expected Behaviors
- No deadlocks or livelocks
- Fair access to critical section
- Distributed pairing without central coordination
- Gradual evening of pair distribution over time

## 📈 Performance Metrics

The implementation tracks:
- Number of successful pairings per process
- Distribution of pairing combinations
- Critical section access fairness
- Message complexity per pairing round

## 🐛 Known Limitations

1. **Even process requirement**: Current implementation assumes equal numbers of artists and engineers
2. **No dynamic joining**: Processes must be known at startup
3. **Simple failure handling**: Limited fault tolerance
4. **Only One pair at the time**: There cant be more pairs than one that create new worlds simultaneously

## 📚 Academic Context

This project demonstrates:
- **Distributed mutual exclusion** algorithms
- **Logical clocks** and event ordering
- **Starvation prevention** techniques
- **Heuristic optimization** in distributed systems
- **MPI programming** paradigms

## 👥 Team Information

*Course*: Distributed Computing  
*Institution*: Poznań University of Technology
*Academic Year*: 2025
