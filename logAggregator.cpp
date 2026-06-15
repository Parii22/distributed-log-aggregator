/*
 * Distributed Log Aggregator Router

 * 1. LogIndex       - Trie for prefix-based log search
 * 2. ErrorTrace     - Stack for sequential error backtracking
 * 3. IngestionLine  - Queue for FIFO log processing
 * 4. AlertLookup    - Hash map for O(1) alert retrieval
 * 5. PrioritySorter - Max-heap to rank logs by severity
 * 6. LogNetwork     - Graph of server dependencies
 * 7. FastestRoute   - Dijkstra shortest path routing
 * 8. ResourceBalancer - Min-heap load balancing
 
 */

#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <stack>
#include <unordered_map>
#include <climits>
#include <tuple>

using namespace std;

// ─── 1. LOG INDEX — Trie ─────────────────────────────────────────────────────
struct TrieNode {
    unordered_map<char, TrieNode*> children;
    vector<string> logs;
};

class LogIndex {
    TrieNode* root = new TrieNode();

    void collect(TrieNode* node, vector<string>& out) {
        for (auto& log : node->logs) out.push_back(log);
        for (auto& [_, child] : node->children) collect(child, out);
    }

public:
  void insert(const string& key, const string& log) {
    TrieNode* node = root;

    for (char c : key) {
        if (!node->children.count(c))
            node->children[c] = new TrieNode();

        node = node->children[c];
    }

    node->logs.push_back(log);
}

    vector<string> search(const string& prefix) {
        auto* node = root;
        for (char c : prefix) {
            auto it = node->children.find(c);
            if (it == node->children.end()) return {};
            node = it->second;
        }
        vector<string> result;
        collect(node, result);
        return result;
    }
};

// Insert: O(L)
// Search: O(L)

// ─── 2. ERROR TRACE — Stack ───────────────────────────────────────────────────
class ErrorTrace {
    stack<string> trace;
public:
    void push(const string& e)  { trace.push(e); }
    void backtrack()            { if (!trace.empty()) trace.pop(); }
    string top() const          { return trace.empty() ? "No errors" : trace.top(); }

    void print() const {
        auto tmp = trace;
        cout << "  Error Trace (top→root):\n";
        while (!tmp.empty()) { cout << "    → " << tmp.top() << "\n"; tmp.pop(); }
    }
};
// Push/Pop: O(1)

// ─── 3. INGESTION LINE — FIFO Queue ──────────────────────────────────────────
class IngestionLine {
    queue<string> line;
public:
    void enqueue(const string& log) { line.push(log); }
    string dequeue() {
        if (line.empty()) return "Queue empty";
        string front = line.front(); line.pop();
        return front;
    }
    int size() const { return (int)line.size(); }
};
// Enqueue/Dequeue: O(1)

// ─── 4. ALERT LOOKUP — Hash Map ──────────────────────────────────────────────
class AlertLookup {
    unordered_map<string, string> alerts;
public:
    void add(const string& id, const string& msg) { alerts[id] = msg; }
    string lookup(const string& id) const {
        auto it = alerts.find(id);
        return it != alerts.end() ? it->second : "Alert not found";
    }
};
// Lookup: Average O(1)

// ─── 5. PRIORITY SORTER — Max-Heap ───────────────────────────────────────────
struct LogEntry {
    int severity;
    string message;
    bool operator<(const LogEntry& o) const { return severity < o.severity; }
};

class PrioritySorter {
    priority_queue<LogEntry> pq;
public:
    void push(int sev, const string& msg) { pq.push({sev, msg}); }
    LogEntry top()  { return pq.top(); }
    void pop()      { pq.pop(); }
    bool empty()    { return pq.empty(); }
};
// Insert: O(log n)
// Extract Max: O(log n)


// ─── 6 & 7. LOG NETWORK — Graph + Dijkstra ───────────────────────────────────
class LogNetwork {
    int V;
    vector<vector<pair<int,int>>> adj;
public:
    LogNetwork(int v) : V(v), adj(v) {}

    void addEdge(int u, int v, int w) {
        adj[u].push_back({v, w});
        adj[v].push_back({u, w});
    }

    vector<int> dijkstra(int src) const {
        vector<int> dist(V, INT_MAX);
        priority_queue<pair<int,int>, vector<pair<int,int>>, greater<>> pq;
        dist[src] = 0;
        pq.push({0, src});
        while (!pq.empty()) {
            auto [d, u] = pq.top(); pq.pop();
            if (d > dist[u]) continue;
            for (auto [v, w] : adj[u])
                if (dist[u] + w < dist[v])
                    pq.push({dist[v] = dist[u] + w, v});
        }
        return dist;
    }

    void printShortestPaths(int src) const {
        auto dist = dijkstra(src);
        cout << "  Shortest latency from server " << src << ":\n";
        for (int i = 0; i < V; i++)
            cout << "    Server " << src << " → " << i
                 << " : " << (dist[i] == INT_MAX ? -1 : dist[i]) << " ms\n";
    }
};
// O((V+E) log V)

// ─── 8. RESOURCE BALANCER — Min-Heap ─────────────────────────────────────────
struct Server {
    int load; string name;
    bool operator>(const Server& o) const { return load > o.load; }
};

class ResourceBalancer {
    priority_queue<Server, vector<Server>, greater<Server>> minHeap;
public:
    void addServer(const string& name) { minHeap.push({0, name}); }

    string assignLog() {
        if (minHeap.empty()) return "No servers";
        auto s = minHeap.top(); minHeap.pop();
        string name = s.name;
        minHeap.push({s.load + 1, name});
        return name;
    }

    void showLoads() const {
        auto tmp = minHeap;
        cout << "\n  Current Server Loads:\n";
        while (!tmp.empty()) {
            cout << "    " << tmp.top().name << " → " << tmp.top().load << " logs\n";
            tmp.pop();
        }
    }
};
// Assign Server: O(log n)

int main() {
    cout << "====================================\n"
         << " Distributed Log Aggregator Router\n"
         << "====================================\n\n";

    // 1. Log Index
    cout << "[1] LOG INDEX (Trie)\n";
    LogIndex idx;
    idx.insert("server1", "server1: CPU spike at 14:02");
    idx.insert("server1", "server1: Memory overflow at 14:05");
    idx.insert("server2", "server2: Disk full at 14:10");
    idx.insert("db",      "db: Connection timeout at 14:15");
    for (const string& prefix : {"server1", "server"}) {
        cout << "  Logs with prefix '" << prefix << "':\n";
        for (auto& r : idx.search(prefix)) cout << "    " << r << "\n";
    }

    // 2. Error Trace
    cout << "\n[2] ERROR TRACE (Stack)\n";
    ErrorTrace et;
    for (auto& e : {"DB connection failed", "Auth service timeout", "API gateway 503"})
        et.push(e);
    et.print();
    cout << "  Backtracking one step...\n";
    et.backtrack();
    et.print();

    // 3. Ingestion Line
    cout << "\n[3] INGESTION LINE (Queue)\n";
    IngestionLine il;
    for (auto& log : {"INFO: Server3 started", "WARN: Server1 high CPU", "ERROR: Server2 disk full"})
        il.enqueue(log);
    cout << "  Queue size: " << il.size() << "\n";
    cout << "  Processing: " << il.dequeue() << "\n";
    cout << "  Processing: " << il.dequeue() << "\n";
    cout << "  Queue size after: " << il.size() << "\n";

    // 4. Alert Lookup
    cout << "\n[4] ALERT LOOKUP (Hash Map)\n";
    AlertLookup al;
    al.add("ALT-001", "CRITICAL: Server2 down!");
    al.add("ALT-002", "WARN: High memory on Server1");
    for (auto& id : {"ALT-001", "ALT-999"})
        cout << "  " << id << " → " << al.lookup(id) << "\n";

    // 5. Priority Sorter
    cout << "\n[5] PRIORITY SORTER (Max-Heap)\n";
    PrioritySorter ps;
    for (auto& [sev, msg] : vector<pair<int,string>>{
        {1, "INFO: Routine check"}, {5, "CRITICAL: Server crash!"},
        {3, "WARN: High load"},    {5, "CRITICAL: DB unreachable!"}})
        ps.push(sev, msg);
    cout << "  Processing by priority:\n";
    while (!ps.empty()) {
        auto [sev, msg] = ps.top(); ps.pop();
        cout << "    [Severity " << sev << "] " << msg << "\n";
    }

    // 6 & 7. Log Network + Dijkstra
    cout << "\n[6 & 7] LOG NETWORK + FASTEST ROUTE (Graph + Dijkstra)\n";
    // 0=Gateway, 1=Server1, 2=Server2, 3=DB, 4=AdminDash
    LogNetwork net(5);
    for (auto& [u, v, w] : vector<tuple<int,int,int>>{
        {0,1,10},{0,2,20},{1,3,15},{2,3,5},{3,4,10},{1,4,50}})
        net.addEdge(u, v, w);
    net.printShortestPaths(0);

    // 8. Resource Balancer
    cout << "\n[8] RESOURCE BALANCER (Min-Heap)\n";
    ResourceBalancer rb;
    for (auto& s : {"ProcessorA", "ProcessorB", "ProcessorC"}) rb.addServer(s);
    for (int i = 1; i <= 7; i++) {
        string log = "log" + to_string(i);
        cout << "  " << log << " → " << rb.assignLog() << "\n";
    }
    rb.showLoads();

    cout << "\n====================================\n"
         << " All systems operational.\n"
         << "====================================\n";
}