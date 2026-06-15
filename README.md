# Distributed Log Aggregator — Project Report

## Project Title

Distributed Log Aggregator Router 

## Problem Statement

LogStream is a centralized monitoring dashboard that collects error logs from thousands of servers. The existing system must support:
- Instant search of logs by server or attribute prefix.
- Sequential error tracing (backtracking through logs that share a trace id).
- High-volume FIFO ingestion with fair processing.
- Constant-time alert lookup by unique id.
- Ranking by severity to trigger immediate alerts.
- Modeling dependencies between servers for root-cause analysis and routing.
- Calculating minimum-latency routes to administrators and balancing processing across worker nodes.

This project implements these features in a compact C++ program to demonstrate the underlying data structures and algorithms.

## Objectives

- Build a Trie-based prefix index for server/attribute searches.
- Maintain a sequential trace map for error traces.
- Implement a FIFO ingestion pipeline.
- Provide constant-time alert lookup using a hash map.
- Rank logs with a priority queue by severity and timestamp.
- Model server dependencies as a graph and compute shortest-latency routes (Dijkstra).
- Implement a simple resource balancer (capacity-aware assignment) for processing.

## System Overview / Architecture

The program is a single-process simulation composed of the following components:

- Ingestion queue (FIFO): holds incoming logs before processing.
- Indexer (Trie): indexes server names to allow prefix search that returns matching log ids.
- Lookup store (unordered_map): holds logs by unique id for O(1) retrieval and alert lookup.
- Trace store (unordered_map<string, vector<string>>): maps trace ids to ordered lists of log ids, enabling sequential backtracking.
- Priority sorter (priority_queue): sorts logs by severity then timestamp for immediate alerting.
- Graph model (adjacency list): represents server dependencies with latencies; used for shortest-path routing.
- Resource balancer: assigns logs to workers based on capacity and current load.

Flow (high-level): ingestion -> processing (index, trace store, priority queue, assignment) -> queries (prefix search, trace lookup, pop priority, shortest path)

Simple ASCII architecture:

Ingest -> [FIFO queue] -> Processor -> {Trie index, Hash lookup, Trace map, Priority queue, Balancer}

Queries: prefix search (Trie) | trace lookup (trace map) | alert lookup (hash) | route calculation (graph)

## Data Structures and Algorithms Used

- Trie (prefix tree): store server strings and accumulate log ids on path nodes for prefix searches.
- unordered_map (hash table): constant-time storage and lookup for logs and trace maps.
- queue: FIFO ingestion pipeline.
- priority_queue: priority sorting by severity (CRITICAL > WARNING > INFO), then by timestamp.
- Dijkstra's algorithm (priority queue + adjacency list): compute shortest-latency path between servers.
- simple vector/list bookkeeping: worker assignment lists and sample storage.

Why these choices
- Trie gives fast prefix queries (time proportional to prefix length + number of matched ids).
- unordered_map gives expected O(1) access for alert lookups and trace collection.
- Dijkstra is the standard algorithm for single-source shortest-path with non-negative weights.

##  Implementation Approach

- The main implementation is in `logAggregator.cpp`.
- Logs are represented by a simple struct with fields: id, timestamp, server, message, severity, trace_id.
- Ingestion: logs are enqueued into a FIFO `queue<Log>`; the `processAll()` method dequeues and indexes each log:
  - store in `lookup` (unordered_map)
  - insert server into `Trie` with the log id
  - append to `traceMap[trace_id]` if trace exists
  - push into `priorityQ` for later alerting
  - assign to a worker using a simple capacity-aware heuristic
- Query helpers:
  - `searchByServerPrefix(prefix)` returns logs whose server matches the prefix using the Trie
  - `getTrace(trace_id)` returns logs in the order they were recorded for a trace
  - `popTopPriority()` returns the highest priority log (not destructively in the demo unless popped)
  - `shortestPath(src,dst)` runs Dijkstra and returns the latency and path

Design notes
- The implementation is synchronous and single-threaded for clarity. Production systems must use concurrency and persistence.
- The Trie stores log ids at each node for demo simplicity; in a realistic system we would store references or use an inverted index to reduce duplication.

## Time and Space Complexity Analysis

Notation: n = number of logs indexed, L = average server string length, k = number of log ids matched by a prefix.

- Insert log into Trie: O(L) time, additional O(1) per node to push id (space increases by O(L) per new server string; storing ids increases space by total inserted ids).
- Trie prefix search: O(P + k log k) where P is prefix length (to reach node) and k is number of returned ids; we sort/unique k ids in the demo.
- Hash lookup (unordered_map): average O(1) time and O(n) space.
- Enqueue / dequeue (queue): O(1) per operation.
- Priority queue push/pop: O(log n) per operation.
- Dijkstra (binary heap): O((V + E) log V) where V = number of servers/nodes in graph and E = edges.
- Worker assignment (scan over workers): O(w) per log where w is number of workers (small in demo).

Space complexity (demo): O(n * L) for trie-inserted ids + O(n) for lookup + O(n) for priority queue in worst case.

##  Execution Steps

Prerequisites: a C++17-capable compiler (g++ or clang++) and make.

1. Open a terminal and change to the `project` directory:

```bash
cd /Users/parii/Desktop/dsa/project
```

2. Build the program (Makefile provided):

```bash
make
```

3. Run the executable:

```bash
./logAggregator
```

If make is not available you can compile directly:

```bash
g++ -std=c++17 -O2 -Wall -o logAggregator logAggregator.cpp
./logAggregator
```

## Sample Inputs and Outputs

The demo program simulates ingestion of these sample logs (shown as id, ts, server, severity, message, trace):

- L1,1001,web-1,INFO,"routine request served"
- L2,1002,web-1,WARNING,"slow response"
- L3,1003,app-1,CRITICAL,"panic: nil pointer",T1
- L4,1004,app-1,WARNING,"retrying downstream",T1
- L5,1005,db-1,CRITICAL,"disk full",T1
- L6,1006,app-2,INFO,"cache miss"
- L7,1007,web-2,INFO,"heartbeat"
- L8,1008,app-2,CRITICAL,"oom killer invoked",T2

Expected sample output (program prints a concise demo report). Exact formatting may vary but an example run prints:

```
Search logs for server prefix 'app':
  L3 app-1 panic: nil pointer
  L4 app-1 retrying downstream
  L6 app-2 cache miss
  L8 app-2 oom killer invoked

Trace T1 sequence:
  L3 [app-1] panic: nil pointer
  L4 [app-1] retrying downstream
  L5 [db-1] disk full

Top 3 priority logs:
  [CRIT] L3 @app-1: panic: nil pointer
  [CRIT] L5 @db-1: disk full
  [CRIT] L8 @app-2: oom killer invoked

Shortest latency path web-1 -> admin: 35 ms via web-1 app-2 db-1 admin 

Worker assignments:
  worker-A (load=5): L1 L3 L4 L6 L8 
  worker-B (load=3): L2 L5 L7 
```

Notes on output
- The prefix search returns log ids matching servers starting with the prefix; the demo returns the log message alongside.
- Trace results are ordered by ingestion (earlier logs first).
- Priority sorting lists CRITICAL logs first, then WARNING, then INFO; ties use timestamp.

## Screenshots
<img width="720" height="675" alt="image" src="https://github.com/user-attachments/assets/1616d94c-325f-44ae-a8b3-153f48d7751f" />
<img width="561" height="438" alt="image" src="https://github.com/user-attachments/assets/4dd39943-054e-4664-a695-02ab19f76daf" />

## Results and Observations

- The demo shows how core components for an aggregator can be built with common data structures:
  - Trie enables quick prefix matching at the cost of additional indexing space.
  - Hash maps provide fast direct alert lookup and trace grouping.
  - Priority queues let the system focus on critical alerts first.
  - Dijkstra allows computing low-latency routing between sources and admin endpoints.
- Observations:
  - The in-memory approach is simple and fast for small datasets but doesn't scale to production traffic without sharding, persistence, and concurrency.
  - The Trie storing log ids at each node is easy to implement but can duplicate many ids; an inverted index or posting lists with compression would be better for scale.
  - The simple balancer works when worker counts are small and stable; production schedulers would use time-based metrics and backpressure.

## Conclusion

This project implements a focused, educational subset of a distributed log aggregator in C++ demonstrating the main algorithmic building blocks: Trie for prefix search, hash-based lookups for alerts and traces, priority queuing for alerting, and graph routing for latency-aware delivery. The code is compact and intended to be extended into a production-grade system with persistence, sharding, concurrency, and networking.

---
