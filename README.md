# README

This repository contains all my work pertaining to the FlyWire Qualification challenge. 

## Challenge:
We have been given 5 connectomic datasets that contain edge lists between neurons. We have to find the largest weakly-connected directed induced subgraph shared across 3 of the 5 datasets and are thus mutually isomorphic.



## Summary of my approach:
I took a hybrid Python-C++ pipeline to aggressively prune the massive datasets into more manageable data. The Python file *Phase2.py* calculates *Complexity scores* for each directed edge. These Complexity scores allow us to isolate massive Hub Neurons to act as starting seeds, thereby eliminating tens of thousands of false positives. I then fed these seed files to the C++ engine at *main.cpp* to conduct the loading and utilisation of the data. It uses coordinate compression to ensure that the massive Neuron IDs won’t eat up exabytes of space and it also utilizes a dual storage memory architecture: Adjacency lists to enforce the “weakly connected” rule paired with flat 1D bit-matrices to ensure instantaneous O(1) structural validation. Once the data is loaded so, I executed a recursive backtracking Depth-First Search (DFS) algorithm to extract the largest possible Neuron circuit. This algorithm has a built-in Timeout system that ensures that horizontal infinite loops are eliminated and also has a way to weed out autapses.

I also built a separate testing algorithm at *validator.cpp* to ensure that all the necessary conditions are met by this circuit.

In the end, the largest weakly-connected directed induced subgraph we found was **237 Neurons** in size and is common to the **FAFB, BANC, and MAOL datasets** and can be found in the file network.csv and  circuit_banc_banc_maol.csv. Thus, we managed to maximise **N to 237**.


## Technicals for the Approach:
My approach can be broadly divided into 5 steps:

### 1. Step 1: Pre-computation:
With an average of nearly 100,000 neurons across the 5 datasets, it is impossible to evaluate every single directed edge as a potential starting point for our DFS algorithm. If you start off the search at a generic neuron with a small number of connections, you will have thousands and thousands of similar neurons in the other 2 datasets and the algorithm will take too long to try and find the true match. The largest circuits, as a rule must contain at least a few neurons that have a large number of upstream and downstream connections. These are Hub neurons are few in number and thus are far more manageable to find matches in the other datasets.

The Python file *phase2.py* takes each dataset one by one and the *process_and_export_seeds* function generates the columns *source neuron ID, target neuron ID, u_in, u_out* (count of upstream and downstream connections of source neuron respectively), *v_in, v_out* (count of upstream and downstream connections of destination neuron, respectively), as well as *complexity*. Complexity is simply the sum of u_in, u_out, v_in, and v_out. 
All of the above processes are conducted using the pandas module and its data structures and the counts mentioned above just use functions to get occurrences of that ID in the file.

Once these values are calculated, the connections are sorted by complexity in descending order and exported as a csv file of the format <databse>_seed.csv. Thus, we end up with 5 seed datasets. When we later go through the combinations one by one, we take the top edge (directed edge with highest complexity score) as the starting point for our algorithm.


### 2. Step 2: Coordinate Compression and Memory Allocation:
Once all this pre-computation is done with, we move on to executing our *main.cpp*. There are 5 datasets, and we have to find a subgraph common to 3 of them. That gives us a total of 10 potential Combinations. We cycle through each combination with using for loops within our main function and the *findMaxSubgraph* function runs here.

The source and destination IDs for the neurons are absolutely massive numbers. If we attempted to use these as indices for a memory array, we would have wasted exabytes of RAM to create an array large enough for the indices.
To get around it, I implemented Coordinate compression using a Hash Map to get contiguous, tightly packed IDs (1,2,3,4,5, ….. N). We had defined a struct *Graph* to hold the memory mapping tools. It contains:

- unordered_map<long long int, int> id_to_index : Links the massive IDs to smaller 	integers.
	
- vector<long long> index_to_id: reverse lookup array used at the very end to translate 	small integers back into IDs for network.csv.

- int total_unique_nodes : a counter for every new neuron discovered.

- getOrAddNode: every neuron is passed to this function, where it checks the hash map to see whether it has seen this neuron ID before. If yes, it simply returns the small integer that was already assigned to it previously. But if no, then it assigns current *total_unique_nodes* value to that ID, stores it in the map, and increments the counter by 1.


The *loadGraphData* function reads the CSVs line by line. As it reads, it runs source and destination IDs through *getOrAddNode* to get their compressed 0-N integers. Since final size of graph remains unknown at this point, these compressed edges are stored in a vector (compressed_edges). Once the entire CSV file is read, the compression is complete and the variable *g.total_unique_nodes* holds the final number of neurons, which is used by the *resize()* function to allocate exactly enough memory. 
Also, we used the try-catch block to prevent stoll from causing issues with text in the headers of the CSV files.

The Adjacency Matrices are now sized to exactly N and sizes the 1D bit-matrix to exactly N*N, so as to get instant index-based access. The vector compressed_edges is looped through to populate these matrices


### Step 3: Dual-Storage
A naive 2D adjacency matrix implementation for the ~130000 neurons of FAFB alone require 16.9 billion cells per dataset and at least 50 GB of space. So I had to find another way.

Graph matching requires two kinds of memory access: Finding neighbours quickly, and verifying non-edges quickly. As mentioned in passing above, the storage is split into Adjacency Lists and a 1D Bit_matrix.
First, the Adjacency Matrices. I built *forwardAdjList* and *backwardAdjList* to instantly find neighboring nodes. Being able to look both upstream and downstream allows us to ensure that the circuit is weakly-connected.

Next, the 1D Bit Matrix. To ensure instantaneous validation of non-edges and self-loops without scanning full arrays, we use this C++ std::vector<bool> container to leverage space optimisation by packing the 8 bytes of a typical integer into a single byte. Thus, by flattening that 2D matrix into a 1D matrix, we managed to bring RAM usage to only 6.3 GB!


## Step 4: Initialisation and DFS
As mentioned previously, we take the topmost of the seed.csv files and locked in a structurally identical  N=2 directed edge across all 3 datasets. Locking in on this N=2 seed provides an optimised anchor to base maximasation around and prevents the algorithm from wasting time on generic neurons.

From the anchor, a recursive Depth-First Search Algorithm (DFS) is employed to spread outwards. DFS ensures that the circuit obtained at the end will certainly be weakly-connected. To avoid a combinatorial explosion, it uses a Frontier, ie. it draws up a strict list of immediate upstream and downstream neighbours of the currently mapped subgraph, queried instantly via Adjacency lists.

Before a candidate triplet is incorporated into the subgraph, it undergoes validation through the *isInducedMatch* function. It verifies that all forward edges, backward edges, non-edges, and self-loops (autapses) match perfectly across all 3 datasets. If it does match, the triplet is pushed into the subgraph and the DFS recurses deeper. If there is a mismatch, the algorithm backtracks, popping the candidate triplet and attempting the next available node on the Frontier.

Ultimately, we must recognise that this is an NP-hard problem and that a massive hub of neurons possess thousands of neighbours, creating billions of pathways. To prevent the DFS from getting trapped in horizontal infinite loops, a hard 60 second time-limit is imposed on the algorithm using std::chrono, after which a runtime exception is thrown and the maximum N found up to that point is saved.

The compressed indices are then translated back into neuron IDs and published as a circuit. 10 such circuits will be published by each of the 10 combinations. **I have actually put the combinations that I found in my latest run in the combinations folder.** The largest such circuit amongst the ten will be named network.csv.




## The Validator
Its quite easy to get perfectly lost in this maze of functions and data structures and, owing to the complexity of the task, elementary mistakes can be made and may cause major inaccuracies. That’s why it is important to build an independent Validator program to ensure that our solution meets all of the challenge’s criteria. Enter *validator.cpp.*

It reads all the rows of network.csv and goes back to the original datasets and checks which edges occur purely between these specific N neurons. It uses those edges to build a new N * N Adjacency matrices for each of the 3 datasets. It then uses a double for loop to check each of the neurons and all of their connections in one matrix and then compares the same from another matrix. 

if (mat1[i][j] != mat2[i][j] || mat1[i][j] != mat3[i][j]) { is_isomorphic = false; // Instant failure! } 

By checking every [i][j] coordinate, it manages to **verify whether both the edges and non-edges are common between the matrices.**
Then, it checks whether it is **weakly connected**.

It runs a DFS algorithm (Depth-First Search) starting from the very first neuron in the circuit and uses a visited array initialized to false to track the progress of the DFS algorithm. Once DFS is complete, the visited array is iterated through and if even one of the cells hadn’t been visited by the DFS algorithm, the circuit would have been declared as failed.

if (matrix[curr][neighbor] || matrix[neighbor][curr])

The || (or operator) ensures that the DFS can spread both forward and backwards through a synapse.

Once we have verified that the circuit is truly common to the 3 datasets and is also weakly connected throughout, we can declare our circuit to be valid!




## How to reproduce the results:
I have made it so that you can download the entire repository yourself and execute it. You can also stick to just downloading the following files and running them on the same folder:
1. main.cpp
2. validator.cpp
3. phase2.py

You also need the 5 datasets that were provided. They must be in the same folder as the above 3 files and also MUST be named in the following manner:
BANC – banc_626_edge_list.csv
FAFB – fafb_783_edge_list.csv
MANC – manc_1.2.1_edge_list.csv
MAOL – maol_1.1_edge_list.csv
MCNS - mcns_0.9_edge_list.csv

First, you must run phase2.py in the terminal like so:

python3 phase2.py

This generates the seed datasets for each of the 5 datasets.

Then, you must run the following to compile and run the C++ Engine *main.cpp*:

g++ -O3 main.cpp -o mcis_solver
./mcis_solver

You will then get a result that gives you the maximum N as well as the 3 datasets that it comes from. Each of the 10 combinations of the dataset will generate its own circuit csv file in the format *circuit_<dataset1>_<dataset2>_<dataset3>.csv.*

Now, go to validator.cpp, and just below the start of the main() function, you will find the variables “string solution file”, “string dataset1”, ”string dataset2”, and “string dataset3”. The first one is already filled by the network.csv, while the next 3 must have the 3 datasets predicted by main.cpp.
Then you must run the following to compile and run validator.cpp to verify the result:
g++ -O3 validator.cpp -o validator
./validator

If it says its a Valid submission, then it satisfies all the given conditions!

