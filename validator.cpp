#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <unordered_set>

using namespace std;

// Loads the generated CSV and maps the 64-bit IDs to a local 0 to N-1 index
bool loadSolution(const string& filename, 
                  unordered_map<long long, int>& map1, 
                  unordered_map<long long, int>& map2, 
                  unordered_map<long long, int>& map3, 
                  int& N) {
    ifstream file(filename);
    if (!file.is_open()) {
        cout << "Error: Could not open solution file " << filename << endl;
        return false;
    }

    string line;
    getline(file, line); // Skip header

    int index = 0;
    while (getline(file, line)) {
        if (line.empty() || line == "\r") continue;
        stringstream ss(line);
        string u1, u2, u3;
        
        if (getline(ss, u1, ',') && getline(ss, u2, ',') && getline(ss, u3, ',')) {
            try {
                map1[stoll(u1)] = index;
                map2[stoll(u2)] = index;
                map3[stoll(u3)] = index;
                index++;
            } catch (...) { continue; } // Skip malformed lines
        }
    }
    N = index;
    return true;
}

// Scans a massive dataset and builds a tiny NxN matrix ONLY for the selected nodes
void buildInducedMatrix(const string& filename, const unordered_map<long long, int>& node_map, vector<vector<bool>>& matrix) {
    ifstream file(filename);
    string line;
    
    while (getline(file, line)) {
        if (line.empty() || line == "\r") continue;
        stringstream ss(line);
        string u_str, v_str;
        
        if (getline(ss, u_str, ',') && getline(ss, v_str, ',')) {
            try {
                long long u = stoll(u_str);
                long long v = stoll(v_str);
                
                // If BOTH nodes are in our solution set, record the edge
                if (node_map.count(u) && node_map.count(v)) {
                    int local_u = node_map.at(u);
                    int local_v = node_map.at(v);
                    matrix[local_u][local_v] = true;
                }
            } catch (...) { continue; } // Skip headers
        }
    }
}

// Checks if the graph is weakly connected (ignoring directed arrows)
void dfsWeakConnectivity(int curr, const vector<vector<bool>>& matrix, vector<bool>& visited, int N) {
    visited[curr] = true;
    for (int neighbor = 0; neighbor < N; neighbor++) {
        // Check both outgoing AND incoming edges for weak connectivity
        if ((matrix[curr][neighbor] || matrix[neighbor][curr]) && !visited[neighbor]) {
            dfsWeakConnectivity(neighbor, matrix, visited, N);
        }
    }
}

int main() {
    string solution_file = "network.csv";
    string dataset1 = "fafb_783_edge_list.csv";
    string dataset2 = "maol_1.1_edge_list.csv";
    string dataset3 = "mcns_0.9_edge_list.csv";

    unordered_map<long long, int> map1, map2, map3;
    int N = 0;

    cout << "Loading solution nodes..." << endl;
    if (!loadSolution(solution_file, map1, map2, map3, N)) return 1;
    cout << "Loaded N = " << N << " matched nodes." << endl;

    // Initialize tiny NxN boolean matrices (Uses almost zero RAM)
    vector<vector<bool>> mat1(N, vector<bool>(N, false));
    vector<vector<bool>> mat2(N, vector<bool>(N, false));
    vector<vector<bool>> mat3(N, vector<bool>(N, false));

    cout << "Extracting induced subgraph from Dataset 1..." << endl;
    buildInducedMatrix(dataset1, map1, mat1);
    
    cout << "Extracting induced subgraph from Dataset 2..." << endl;
    buildInducedMatrix(dataset2, map2, mat2);
    
    cout << "Extracting induced subgraph from Dataset 3..." << endl;
    buildInducedMatrix(dataset3, map3, mat3);

    // ---------------------------------------------------------
    // TEST 1: MUTUAL ISOMORPHISM (Edges & Non-edges)
    // ---------------------------------------------------------
    cout << "\n[TEST 1] Verifying Mutual Isomorphism..." << endl;
    bool is_isomorphic = true;
    int edge_count = 0;

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            if (mat1[i][j] != mat2[i][j] || mat1[i][j] != mat3[i][j]) {
                is_isomorphic = false;
                cout << "  -> FAIL: Mismatch found at local index " << i << " -> " << j << endl;
                break;
            }
            if (mat1[i][j]) edge_count++;
        }
        if (!is_isomorphic) break;
    }

    if (is_isomorphic) {
        cout << "  -> PASS: Subgraphs are perfectly isomorphic!" << endl;
    }

    // ---------------------------------------------------------
    // TEST 2: WEAK CONNECTIVITY
    // ---------------------------------------------------------
    cout << "\n[TEST 2] Verifying Weak Connectivity..." << endl;
    vector<bool> visited(N, false);
    
    // Start DFS from node index 0
    dfsWeakConnectivity(0, mat1, visited, N);
    
    bool is_connected = true;
    for (int i = 0; i < N; i++) {
        if (!visited[i]) {
            is_connected = false;
            break;
        }
    }

    if (is_connected) {
        cout << "  -> PASS: The circuit is fully weakly connected!" << endl;
    } else {
        cout << "  -> FAIL: The circuit is broken into disconnected islands." << endl;
    }

    // Final Verdict
    cout << "\n========================================" << endl;
    if (is_isomorphic && is_connected) {
        cout << "FINAL VERDICT: VALID SUBMISSION" << endl;
        cout << "The solution strictly adheres to all challenge rules." << endl;
    } else {
        cout << "FINAL VERDICT: INVALID SUBMISSION" << endl;
        cout << "Check the failure logs above." << endl;
    }
    cout << "========================================" << endl;

    return 0;
}
