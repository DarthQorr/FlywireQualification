#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <chrono> // Required for the Time Budget
#include <stdexcept>

using namespace std;

struct Graph {
    // Maps the massive 64-bit FlyWire ID to our tight 0-N index
    unordered_map<long long, int> id_to_index;
    
    // Maps the 0-N index back to the FlyWire ID (for printing the final answer)
    vector<long long> index_to_id; 

    vector<vector<int>> forwardAdjList;  // A -> B
    vector<vector<int>> backwardAdjList; // B -> A
    vector<bool> bitMatrix;
    int total_unique_nodes = 0;

    // Helper to compress IDs on the fly
    int getOrAddNode(long long raw_id) {
        if (id_to_index.find(raw_id) == id_to_index.end()) {
            id_to_index[raw_id] = total_unique_nodes;
            index_to_id.push_back(raw_id);
            total_unique_nodes++;
        }
        return id_to_index[raw_id];
    }
};

inline long long getMatrixIndex(int source, int target, int total_nodes) {
    return ((long long)source * total_nodes) + target;
}

// A simple struct to hold our N=2 starting seed
struct SeedTriplet {
    long long d1_u, d1_v; 
    long long d2_u, d2_v; 
    long long d3_u, d3_v; 
};

SeedTriplet initializeSeed(const string& seedFile1, const string& seedFile2, const string& seedFile3) {
    SeedTriplet seed;
    
    // Helper lambda to read the top row of a Python seed file
    auto getTopEdge = [](const string& filename, long long& u, long long& v) {
        string line; // MOVED INSIDE THE LAMBDA
        ifstream file(filename);
        if (!file.is_open()) return;
        getline(file, line); // Skip header
        if (getline(file, line)) {
            stringstream ss(line);
            string u_str, v_str;
            getline(ss, u_str, ',');
            getline(ss, v_str, ',');
            u = stoll(u_str);
            v = stoll(v_str);
        }
    };
    
    // Read the top edge from each of the 3 active datasets
    getTopEdge(seedFile1, seed.d1_u, seed.d1_v);
    getTopEdge(seedFile2, seed.d2_u, seed.d2_v);
    getTopEdge(seedFile3, seed.d3_u, seed.d3_v);

    return seed;
}



void loadGraphData(const string& filename, Graph& g) {
    ifstream file(filename);
    string line;
    long long raw_u, raw_v;
    
    // Temporarily store the edges until we know the absolute total number of nodes
    vector<pair<int, int>> compressed_edges;
    
    while (getline(file, line)) {
        // Skip empty lines immediately
        if (line.empty() || line == "\r") continue; 

        stringstream ss(line);
        string u_str, v_str;
        
        if(getline(ss, u_str, ',') && getline(ss, v_str, ',')) {
            try {
                // Attempt to convert the strings to 64-bit integers
                raw_u = stoll(u_str);
                raw_v = stoll(v_str);
                
                // Compress the massive IDs down to 0, 1, 2...
                int comp_u = g.getOrAddNode(raw_u);
                int comp_v = g.getOrAddNode(raw_v);
                
                compressed_edges.push_back({comp_u, comp_v});
            } catch (...) {
                // If stoll fails (e.g., because it's reading a Header like "source, target"),
                // simply ignore this line and continue to the next row.
                continue; 
            }
        }
    }
    
    // Now that we know EXACTLY how many unique nodes exist, we allocate memory safely
    g.forwardAdjList.resize(g.total_unique_nodes);
    g.backwardAdjList.resize(g.total_unique_nodes); 
    
    long long matrix_size = (long long)g.total_unique_nodes * g.total_unique_nodes;
    g.bitMatrix.resize(matrix_size, false);
    
    for (auto edge : compressed_edges) {
        g.forwardAdjList[edge.first].push_back(edge.second);
        g.backwardAdjList[edge.second].push_back(edge.first); // Track the reverse path
        g.bitMatrix[getMatrixIndex(edge.first, edge.second, g.total_unique_nodes)] = true;
    }
}

// This is the placeholder for Phases 2 through 5.
// =====================================================================
// PHASE 5: THE STRICT INDUCED CHECK
// =====================================================================
// =====================================================================
// PHASE 5: THE STRICT INDUCED CHECK
// =====================================================================
bool isInducedMatch(int c1, int c2, int c3, 
                    const vector<int>& sub1, const vector<int>& sub2, const vector<int>& sub3,
                    const Graph& g1, const Graph& g2, const Graph& g3) {
    
    // --- NEW: THE SELF-LOOP (AUTAPSE) CHECK ---
    bool self1 = g1.bitMatrix[getMatrixIndex(c1, c1, g1.total_unique_nodes)];
    bool self2 = g2.bitMatrix[getMatrixIndex(c2, c2, g2.total_unique_nodes)];
    bool self3 = g3.bitMatrix[getMatrixIndex(c3, c3, g3.total_unique_nodes)];
    
    // If the self-loop existence isn't identical across all 3, reject them.
    if (self1 != self2 || self1 != self3) return false;
    // ------------------------------------------

    // We must check the new candidate against EVERY node already in the subgraph
    for (size_t i = 0; i < sub1.size(); i++) {
        int m1 = sub1[i];
        int m2 = sub2[i];
        int m3 = sub3[i];

        // 1. Check Forward Edges (Candidate -> Mapped Node) & Non-Edges
        bool f1 = g1.bitMatrix[getMatrixIndex(c1, m1, g1.total_unique_nodes)];
        bool f2 = g2.bitMatrix[getMatrixIndex(c2, m2, g2.total_unique_nodes)];
        bool f3 = g3.bitMatrix[getMatrixIndex(c3, m3, g3.total_unique_nodes)];
        
        if (f1 != f2 || f1 != f3) return false;

        // 2. Check Backward Edges (Mapped Node -> Candidate) & Non-Edges
        bool b1 = g1.bitMatrix[getMatrixIndex(m1, c1, g1.total_unique_nodes)];
        bool b2 = g2.bitMatrix[getMatrixIndex(m2, c2, g2.total_unique_nodes)];
        bool b3 = g3.bitMatrix[getMatrixIndex(m3, c3, g3.total_unique_nodes)];
        
        if (b1 != b2 || b1 != b3) return false;
    }
    
    return true; 
}

// =====================================================================
// PHASE 4: THE WEAKLY CONNECTED FRONTIER (Recursive Backtracking DFS)
// =====================================================================
void expandSubgraph(Graph& g1, Graph& g2, Graph& g3, 
                    vector<int>& sub1, vector<int>& sub2, vector<int>& sub3,
                    vector<bool>& in_sub1, vector<bool>& in_sub2, vector<bool>& in_sub3,
                    int& max_N, chrono::time_point<chrono::high_resolution_clock> start_time,
                    vector<int>& best_sub1, vector<int>& best_sub2, vector<int>& best_sub3) {
                            
    // 1. The Time Budget: Abort this branch if it takes longer than 60 seconds
    auto current_time = chrono::high_resolution_clock::now();
    if (chrono::duration_cast<chrono::seconds>(current_time - start_time).count() > 180) return;

    // Track the largest N found so far
    if (sub1.size() > max_N) {
        max_N = sub1.size();
        best_sub1 = sub1; 
        best_sub2 = sub2; 
        best_sub3 = sub3;
    }

    // 2. Build the Frontier from Dataset 1
    // We pick the very last added node in sub1 to act as our "Anchor" to grow outward
    int anchor_idx = sub1.size() - 1;
    int u1 = sub1[anchor_idx];
    int u2 = sub2[anchor_idx];
    int u3 = sub3[anchor_idx];

    // Combine forward and backward neighbors of u1 to satisfy "Weakly Connected"
    vector<int> frontier_g1;
    for (int neighbor : g1.forwardAdjList[u1]) frontier_g1.push_back(neighbor);
    for (int neighbor : g1.backwardAdjList[u1]) frontier_g1.push_back(neighbor);

    // 3. Test Candidates
    for (int c1 : frontier_g1) {
        auto current_time = chrono::high_resolution_clock::now();
        if (chrono::duration_cast<chrono::seconds>(current_time - start_time).count() > 180) {
            throw runtime_error("Timeout"); // Instantly shatters all loops and call stacks!
        }

        if (in_sub1[c1]) continue; // Skip if already in the subgraph

        // Use the Anchor Node Optimization to find c2 and c3!
        // c2 MUST be in the weakly connected frontier of u2
        vector<int> frontier_g2;
        for (int n : g2.forwardAdjList[u2]) frontier_g2.push_back(n);
        for (int n : g2.backwardAdjList[u2]) frontier_g2.push_back(n);

        vector<int> frontier_g3;
        for (int n : g3.forwardAdjList[u3]) frontier_g3.push_back(n);
        for (int n : g3.backwardAdjList[u3]) frontier_g3.push_back(n);

        // Try to map c1 to a valid c2
        for (int c2 : frontier_g2) {
            if (in_sub2[c2]) continue;

            // Try to map c1 and c2 to a valid c3
            for (int c3 : frontier_g3) {
                if (in_sub3[c3]) continue;

                // PHASE 5: Execute the strict induced check before advancing
                if (isInducedMatch(c1, c2, c3, sub1, sub2, sub3, g1, g2, g3)) {
                    
                    // --- THE RECURSIVE DIVE (Push) ---
                    sub1.push_back(c1); in_sub1[c1] = true;
                    sub2.push_back(c2); in_sub2[c2] = true;
                    sub3.push_back(c3); in_sub3[c3] = true;

                    // Recurse deeper into the network (Now passing the best_sub arrays!)
                    expandSubgraph(g1, g2, g3, sub1, sub2, sub3, in_sub1, in_sub2, in_sub3, max_N, start_time, best_sub1, best_sub2, best_sub3);
                    // --- THE BACKTRACK (Pop) ---
                    sub1.pop_back(); in_sub1[c1] = false;
                    sub2.pop_back(); in_sub2[c2] = false;
                    sub3.pop_back(); in_sub3[c3] = false;
                }
            }
        }
    }
}
// It will eventually load the CSVs, build the bit-matrices, and run the DFS.
int findMaxSubgraph(const string& file1, const string& file2, const string& file3) {
    cout << "  [Memory Allocated] Initializing 6.3 GB of RAM for bit-matrices..." << endl;
    
    Graph g1, g2, g3;
    
    cout << "  Loading " << file1 << "..." << endl;
    loadGraphData(file1, g1);
    cout << "  Loading " << file2 << "..." << endl;
    loadGraphData(file2, g2);
    cout << "  Loading " << file3 << "..." << endl;
    loadGraphData(file3, g3);

    cout << "  [Phase 3] Finding N=2 Seed Match..." << endl;
    
    // The Python-generated files
    string seed1 = file1.substr(0, file1.find_last_of('.')) + "_seeds.csv";
    string seed2 = file2.substr(0, file2.find_last_of('.')) + "_seeds.csv";
    string seed3 = file3.substr(0, file3.find_last_of('.')) + "_seeds.csv";
    
    SeedTriplet start_edge = initializeSeed(seed1, seed2, seed3);
    
    cout << "  Seed Locked: D1(" << start_edge.d1_u << "->" << start_edge.d1_v << "), "
         << "D2(" << start_edge.d2_u << "->" << start_edge.d2_v << "), "
         << "D3(" << start_edge.d3_u << "->" << start_edge.d3_v << ")" << endl;

// MUST TRANSLATE RAW PYTHON IDs TO C++ COMPRESSED INDICES!
    int start_c1_u = g1.id_to_index[start_edge.d1_u];
    int start_c1_v = g1.id_to_index[start_edge.d1_v];
    
    int start_c2_u = g2.id_to_index[start_edge.d2_u];
    int start_c2_v = g2.id_to_index[start_edge.d2_v];
    
    int start_c3_u = g3.id_to_index[start_edge.d3_u];
    int start_c3_v = g3.id_to_index[start_edge.d3_v];

    // Initialize Tracking Arrays (O(V) memory)
    vector<int> sub1, sub2, sub3;
    vector<bool> in_sub1(g1.total_unique_nodes, false);
    vector<bool> in_sub2(g2.total_unique_nodes, false);
    vector<bool> in_sub3(g3.total_unique_nodes, false);

    // Safely push the COMPRESSED indices
    sub1.push_back(start_c1_u); in_sub1[start_c1_u] = true;
    sub1.push_back(start_c1_v); in_sub1[start_c1_v] = true;

    sub2.push_back(start_c2_u); in_sub2[start_c2_u] = true;
    sub2.push_back(start_c2_v); in_sub2[start_c2_v] = true;

    sub3.push_back(start_c3_u); in_sub3[start_c3_u] = true;
    sub3.push_back(start_c3_v); in_sub3[start_c3_v] = true;

    int max_N_found = 2; // Starting size
    auto start_time = chrono::high_resolution_clock::now();

    // DECLARE THE ARRAYS HERE SO THEY EXIST!
    vector<int> best_sub1, best_sub2, best_sub3;

    cout << "  [Phase 4 & 5] Launching Backtracking DFS..." << endl;
    
    // Launch the algorithm (NOW PASSING THE 3 BEST VECTORS)
    try {
        expandSubgraph(g1, g2, g3, sub1, sub2, sub3, in_sub1, in_sub2, in_sub3, max_N_found, start_time, best_sub1, best_sub2, best_sub3);
    } catch (const exception& e) {
        cout << "  [Time Budget Reached] 60 seconds elapsed. Halting branch." << endl;
    }
    
    cout << "  [DFS Complete] Largest Isomorphic Subgraph N = " << max_N_found << endl;    cout << "  [Memory Freed] Search complete for this triplet." << endl;

    auto getCleanName = [](string filename) {
        size_t pos = filename.find("_edge_list");
        if (pos != string::npos) return filename.substr(0, pos);
        return filename;
    };

    string clean1 = getCleanName(file1);
    string clean2 = getCleanName(file2);
    string clean3 = getCleanName(file3);

    // This creates a name like: circuit_fafb_783_banc_626_maol_1.1.csv
    string out_filename = "circuit_" + clean1 + "_" + clean2 + "_" + clean3 + ".csv";
    ofstream out(out_filename);
    
    // Set the headers to the actual dataset names!
    out << clean1 << "," << clean2 << "," << clean3 << "\n";
    
    for(size_t i = 0; i < best_sub1.size(); i++){
        // Translate the 0-N index back to the 64-bit ID
        out << g1.index_to_id[best_sub1[i]] << "," 
            << g2.index_to_id[best_sub2[i]] << "," 
            << g3.index_to_id[best_sub3[i]] << "\n";
    }
    out.close();
    cout << "  [Export] Saved branch circuit to " << out_filename << endl;


    return max_N_found;
}



int main() {
    // The 5 provided edge list files
    vector<string> datasets = {
        "fafb_783_edge_list.csv",
        "banc_626_edge_list.csv",
        "manc_1.2.1_edge_list.csv",
        "maol_1.1_edge_list.csv",
        "mcns_0.9_edge_list.csv"
    };

    int global_max_N = 0;
    vector<string> best_combination;

    cout << "Starting Maximum Common Induced Subgraph Search..." << endl;
    cout << "Total Combinations to check: 10\n" << endl;

    // The Combinatorial Outer Loop (5 Choose 3)
    int combination_counter = 1;
    for (int i = 0; i < datasets.size() - 2; ++i) {
        for (int j = i + 1; j < datasets.size() - 1; ++j) {
            for (int k = j + 1; k < datasets.size(); ++k) {
                
                cout << "--- Combination " << combination_counter++ << "/10 ---" << endl;
                cout << "Analyzing: " << datasets[i] << ", " << datasets[j] << ", " << datasets[k] << endl;

                // Execute the core algorithm for this specific triplet
                int current_N = findMaxSubgraph(datasets[i], datasets[j], datasets[k]);

                // Track the overall winner
                if (current_N > global_max_N) {
                    global_max_N = current_N;
                    best_combination = {datasets[i], datasets[j], datasets[k]};
                }
                cout << endl;
            }
        }
    }

    // Final Output Logging
    cout << "========================================" << endl;
    cout << "SEARCH COMPLETE" << endl;
    cout << "Maximum N found: " << global_max_N << endl;
    if (global_max_N > 0) {
        cout << "Winning Triplet: " 
             << best_combination[0] << ", " 
             << best_combination[1] << ", " 
             << best_combination[2] << endl;
    }
    cout << "========================================" << endl;


    if(global_max_N > 0) {
        auto getCleanName = [](string filename) {
            size_t pos = filename.find("_edge_list");
            if (pos != string::npos) return filename.substr(0, pos);
            return filename;
        };

        // Reconstruct the winning filename
        string winning_filename = "circuit_" + 
                                  getCleanName(best_combination[0]) + "_" + 
                                  getCleanName(best_combination[1]) + "_" + 
                                  getCleanName(best_combination[2]) + ".csv";

        // Read the winning file and copy it to network.csv
        ifstream src(winning_filename, ios::binary);
        ofstream dst("network.csv", ios::binary);
        dst << src.rdbuf();

        cout << "-> Successfully copied the global winner to network.csv" << endl;
    }


    return 0;
}
