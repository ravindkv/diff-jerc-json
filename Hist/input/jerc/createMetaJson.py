#!/usr/bin/env python3
"""
create_metadata.py

This script generates a metadata JSON file that maps base correction keys
to their corresponding tags in two different JEC JSON files using Python's json library.

Usage:
    python create_metadata.py --v1 Winter24Prompt24_V1.json --v2 Winter24Prompt24_V2.json --output metadata.json
"""

import json
import argparse
import re
from collections import defaultdict

baseDir = "input/jerc"

def extract_base_key(tag_name):
    """
    Extract the base key from the full tag name.

    Example:
        Input: "Winter24Prompt24_RunBCD_V1_DATA_L1FastJet_AK4PFPuppi"
        Output: "DATA_L1FastJet_AK4PFPuppi"
    """
    # Regular expression to capture the part after 'DATA_' or 'MC_'
    match = re.search(r'(DATA|MC)_.+', tag_name)
    if match:
        return match.group(0)
    else:
        # If the pattern doesn't match, return the full tag name
        return tag_name

def load_tags(json_file):
    """
    Load tag names from a JEC JSON file using Python's json library.

    Args:
        json_file (str): Path to the JSON file.

    Returns:
        set: A set of tag names.
    """
    with open(json_file, 'r') as f:
        data = json.load(f)
    
    # Assuming the JSON structure has a 'corrections' key which is a list of corrections
    tags = set()
    for correction in data.get('corrections', []):
        tag_name = correction.get('name')
        if tag_name:
            tags.add(tag_name)
    
    return tags

def create_metadata(json_v1, json_v2):
    """
    Create a metadata dictionary mapping base keys to their corresponding tags in V1 and V2.

    Args:
        json_v1 (str): Path to V1 JSON file.
        json_v2 (str): Path to V2 JSON file.

    Returns:
        dict: Metadata dictionary.
    """
    tags_v1 = load_tags(json_v1)
    tags_v2 = load_tags(json_v2)

    base_keys_v1 = {extract_base_key(tag): tag for tag in tags_v1}
    base_keys_v2 = {extract_base_key(tag): tag for tag in tags_v2}

    # Identify all unique base keys
    all_base_keys = set(base_keys_v1.keys()).union(set(base_keys_v2.keys()))

    metadata = defaultdict(list)

    for base_key in all_base_keys:
        # Append V1 tag if exists
        if base_key in base_keys_v1:
            metadata[base_key].append(["%s/%s"%(baseDir, json_v1), base_keys_v1[base_key]])
        else:
            metadata[base_key].append(["%s/%s"%(baseDir, json_v1), None])  # None indicates missing tag

        # Append V2 tag if exists
        if base_key in base_keys_v2:
            metadata[base_key].append(["%s/%s"%(baseDir, json_v2), base_keys_v2[base_key]])
        else:                                                    
            metadata[base_key].append(["%s/%s"%(baseDir, json_v2), None])

    return metadata

def save_metadata(metadata, output_file):
    """
    Save the metadata dictionary to a JSON file.

    Args:
        metadata (dict): Metadata dictionary.
        output_file (str): Path to the output JSON file.
    """
    with open(output_file, 'w') as f:
        json.dump(metadata, f, indent=4)
    print(f"Metadata saved to {output_file}")

def main():
    parser = argparse.ArgumentParser(description="Create metadata JSON for JEC tags comparison.")
    parser.add_argument('--v1', required=True, help="Path to V1 JSON file.")
    parser.add_argument('--v2', required=True, help="Path to V2 JSON file.")
    parser.add_argument('--output', default='metadata.json', help="Output metadata JSON file name.")

    args = parser.parse_args()

    metadata = create_metadata(args.v1, args.v2)
    save_metadata(metadata, args.output)

if __name__ == "__main__":
    main()

