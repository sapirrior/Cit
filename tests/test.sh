#!/bin/bash
# Cit Hardcore Test Suite

# Set absolute paths
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CIT_BIN="$SCRIPT_DIR/../build/cit"
TEST_ENV="$SCRIPT_DIR/sandbox"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m'

function check_status {
    if [ $1 -eq 0 ]; then
        echo -e "${GREEN}[PASS]${NC} $2"
    else
        echo -e "${RED}[FAIL]${NC} $2"
        exit 1
    fi
}

echo "=== STARTING CIT HARDCORE TEST SUITE ==="

# 1. Build
cd "$SCRIPT_DIR/.."
echo "Building Cit..."
make clean > /dev/null
make > /dev/null
check_status $? "Build Cit"

# 2. Setup Sandbox
echo "Setting up test sandbox in $TEST_ENV..."
rm -rf "$TEST_ENV"
mkdir -p "$TEST_ENV"
cd "$TEST_ENV"

# 3. Core Workflow
$CIT_BIN init
check_status $? "Repository initialization"

$CIT_BIN config -u "Hardcore Tester" -e "hardcore@cit.io"
check_status $? "User configuration"

# 4. Stress Test: Binary Search
echo "Generating 1000 files..."
mkdir -p bulk_data
for i in {1..1000}; do
    echo "Stress test file $i" > "bulk_data/file_$i.txt"
done
echo "Staging 1000 files..."
time $CIT_BIN add .
check_status $? "Bulk add (1000 files)"

$CIT_BIN status | grep "bulk_data/file_500.txt" > /dev/null
check_status $? "Binary Search lookup"

# 5. Commit & Storage
$CIT_BIN commit "Massive commit"
check_status $? "Massive commit (CIT-LZ)"

# 6. Recursive Branches
echo "Testing hierarchical branches..."
$CIT_BIN branch feature/test-1
check_status $? "Hierarchical branch creation"

$CIT_BIN branch | grep "feature/test-1" > /dev/null
check_status $? "Recursive branch listing"

# 7. Deep Nesting
mkdir -p deep/nested/dir/structure
echo "Nested data" > deep/nested/dir/structure/data.txt
$CIT_BIN add .
$CIT_BIN commit "Nested commit"
check_status $? "Deep nesting support"

# 8. Smart Checkout & Branch Switching
echo "Testing branch switching..."
$CIT_BIN branch feature/stable-v1
check_status $? "Branch creation"

$CIT_BIN checkout feature/stable-v1
check_status $? "Switch to branch feature/stable-v1"

# Verify HEAD
grep "ref: refs/heads/feature/stable-v1" .cit/HEAD > /dev/null
check_status $? "HEAD correctly points to new branch"

# Create a commit on the new branch
echo "Feature content" > feature.txt
$CIT_BIN add feature.txt
$CIT_BIN commit "Feature commit"

# Switch back to main
$CIT_BIN checkout main
check_status $? "Switch back to main"
grep "ref: refs/heads/main" .cit/HEAD > /dev/null
check_status $? "HEAD correctly points to main"

# 9. Delta Compression Verification (in a fresh repo to isolate tree overhead)
echo "Testing Delta Compression..."
mkdir -p delta_test_repo
cd delta_test_repo
$CIT_BIN init > /dev/null

echo "Large base content for delta testing. Repeating to ensure size." > delta_test.txt
for i in {1..100}; do echo "Line $i: some repetitive data" >> delta_test.txt; done

$CIT_BIN add delta_test.txt
$CIT_BIN commit "Delta base" > /dev/null
BASE_OBJ_SIZE=$(du -sb .cit/objects | cut -f1)

echo "Adding one line for delta" >> delta_test.txt
$CIT_BIN add delta_test.txt
$CIT_BIN commit "Delta update 1" > /dev/null

echo "Adding another line for delta" >> delta_test.txt
$CIT_BIN add delta_test.txt
$CIT_BIN commit "Delta update 2" > /dev/null

FINAL_OBJ_SIZE=$(du -sb .cit/objects | cut -f1)
INCREASE=$((FINAL_OBJ_SIZE - BASE_OBJ_SIZE))

# With 1 file, the tree and commit overhead is small (~200 bytes per commit)
# Delta should be very efficient for this file.
echo "Object store increased by $INCREASE bytes for 2 delta commits."
if [ $INCREASE -lt 2000 ]; then
    echo -e "${GREEN}[PASS]${NC} Delta compression active and efficient"
else
    echo -e "${RED}[FAIL]${NC} Delta compression inefficient or inactive (increase: $INCREASE)"
    exit 1
fi

cd ..

echo "---------------------------------------"
echo -e "${GREEN}HARDCORE TEST SUITE COMPLETED SUCCESSFULLY${NC}"
echo "---------------------------------------"
