#!/usr/bin/python3
import os
import sys

ci_tests_dir = os.path.dirname(__file__)
project_dir = os.path.dirname(ci_tests_dir)
witness_node_path = os.path.join(project_dir, 'programs', 'witness_node', 'witness_node')
cli_wallet_path = os.path.join(project_dir, 'programs', 'cli_wallet', 'cli_wallet')

def main():
  if not os.path.exists(witness_node_path):
    print("witness_node program not exist")
    exit(1)
  print("witness_node found")

if __name__ == '__main__':
  main()
