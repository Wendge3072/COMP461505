#!/bin/bash

# Define arrays for each parameter
cache_sizes=(8 16 32 64)
cache_associations=(1 2 4 8)
block_sizes=(16 32 64 128)

# Define output file
output_file="all_results2.txt"

# Clear the output file
> "$output_file"

# Loop through all combinations
for cache_size in "${cache_sizes[@]}"; do
  for cache_asso in "${cache_associations[@]}"; do
    for block_size in "${block_sizes[@]}"; do
      # Define the command with parameters
      command="./lab4 --cache_size $cache_size --cache_asso $cache_asso --block_size $block_size"
      
      # Write separator and command info to output file
      echo "Running: $command" >> "$output_file"
      echo "Parameters: cache_size=$cache_size, cache_asso=$cache_asso, block_size=$block_size" >> "$output_file"
      
      # Execute the command and append output to output file
      $command >> "$output_file"
      
      echo "Output for cache_size=$cache_size, cache_asso=$cache_asso, block_size=$block_size appended to $output_file"
      echo >> "$output_file"  # Add an empty line for better readability
    done
  done
done

echo "All results have been saved to $output_file"

