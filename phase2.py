import pandas as pd
import os

def process_and_export_seeds(input_csv, output_csv):
    print(f"Loading {input_csv}...")
    
    # Read the edge list. Assuming raw CSVs have no headers, just two columns.
    df = pd.read_csv(input_csv, header=None, names=['source', 'target'])

    print("Calculating degree signatures...")
    # Count occurrences to get the exact out-degree and in-degree for every neuron
    out_deg = df['source'].value_counts().to_dict()
    in_deg = df['target'].value_counts().to_dict()

    # Map these degree signatures back to every single edge
    # u = source neuron, v = target neuron
    df['u_out'] = df['source'].map(out_deg).fillna(0).astype(int)
    df['u_in'] = df['source'].map(in_deg).fillna(0).astype(int)
    df['v_out'] = df['target'].map(out_deg).fillna(0).astype(int)
    df['v_in'] = df['target'].map(in_deg).fillna(0).astype(int)

    print("Scoring and sorting edges...")
    # The Complexity Score: The sum of all degrees involved in this specific edge.
    # Edges connecting two massive hubs will float to the top.
    df['complexity_score'] = df['u_out'] + df['u_in'] + df['v_out'] + df['v_in']

    # Sort the dataset by the complexity score in descending order
    df_sorted = df.sort_values(by='complexity_score', ascending=False)

    print(f"Exporting sorted seeds to {output_csv}...\n")
    # Export the clean, sorted data for C++ to ingest
    df_sorted.to_csv(output_csv, index=False)

# The 5 FlyWire dataset edge lists
datasets = [
    "fafb_783_edge_list.csv",
    "banc_626_edge_list.csv",
    "manc_1.2.1_edge_list.csv",
    "maol_1.1_edge_list.csv",
    "mcns_0.9_edge_list.csv"
]

# Execute the pipeline for all datasets
for dataset in datasets:
    if os.path.exists(dataset):
        output_name = dataset.replace(".csv", "_seeds.csv")
        process_and_export_seeds(dataset, output_name)
    else:
        print(f"File not found: {dataset}. Skipping.")

print("Phase 2 Complete. Data is ready for C++.")