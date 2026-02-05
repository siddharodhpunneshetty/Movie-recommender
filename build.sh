#!/bin/bash
# =============================================================================
# build.sh - Auto-compile C Recommendation Engine for Linux Deployment
# =============================================================================
# This script compiles the C program during cloud deployment.
# Runs automatically on Render/Railway before starting the Flask server.
# =============================================================================

set -e  # Exit immediately if any command fails

echo "=========================================="
echo "Building Movie Recommender Engine..."
echo "=========================================="

# Navigate to c_engine directory where recommender.c is located
cd c_engine

echo "[1/3] Checking for source file..."
if [ ! -f "recommender.c" ]; then
    echo "ERROR: recommender.c not found!"
    exit 1
fi
echo "      Found recommender.c"

echo "[2/3] Compiling C program..."
gcc -o recommender recommender.c -lm

echo "[3/3] Verifying compilation..."
if [ ! -f "recommender" ]; then
    echo "ERROR: Compilation failed - executable not created!"
    exit 1
fi
echo "      Executable 'recommender' created successfully"

# Make executable (just in case)
chmod +x recommender

echo "=========================================="
echo "Build completed successfully!"
echo "=========================================="
