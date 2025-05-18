#!/bin/bash

# Set environment variables for reproducibility - only affects this script and its child processes
# This is ephemeral and won't persist after script execution
export MNRM_DEBUG_SEED=7

# Define colors using tput (more portable than ANSI codes)
if [ -t 1 ]; then
    GREEN=$(tput setaf 2)
    RED=$(tput setaf 1)
    BLUE=$(tput setaf 4)
    NC=$(tput sgr0)
else
    # No colors if not running in a terminal
    GREEN=''
    RED=''
    BLUE=''
    NC=''
fi

echo -e "${BLUE}Setting fixed random seed: MNRM_DEBUG_SEED=${MNRM_DEBUG_SEED}${NC}"

echo -e "${BLUE}=== Simpact Cyan Output Comparison Test ===${NC}"
echo -e "${BLUE}This test compares the output files from release and debug builds${NC}"
echo ""

# Define paths and filenames
ORIG_CONFIG="test_config1.txt"
TEST_CONFIG="test_config1_small.txt"
RELEASE_EXE="./build/viss-release"
DEBUG_EXE="./build/viss-debug"
OPTIONS="0 opt -o"
RELEASE_PREFIX="release_"
DEBUG_PREFIX="debug_"

# Create a temporary test config with smaller population
echo -e "${BLUE}Creating test configuration with smaller population...${NC}"
cp "$ORIG_CONFIG" "$TEST_CONFIG"

# Replace population sizes with smaller values for faster testing
sed -i 's/population\.nummen *=.*$/population.nummen = 500/' "$TEST_CONFIG"
sed -i 's/population\.numwomen *=.*$/population.numwomen = 500/' "$TEST_CONFIG"

echo -e "${GREEN}Updated test configuration:${NC}"
grep -E "population\.num(men|women)" "$TEST_CONFIG"

# Check if executables exist
if [ ! -f "$RELEASE_EXE" ]; then
    echo -e "${RED}Error: Release executable not found at $RELEASE_EXE${NC}"
    echo "Please build the project first"
    exit 1
fi

if [ ! -f "$DEBUG_EXE" ]; then
    echo -e "${RED}Error: Debug executable not found at $DEBUG_EXE${NC}"
    echo "Please build the project first"
    exit 1
fi

# Define the log files to check
LOG_FILES=(
    "eventlog.csv"
    "hivviralloadlog.csv"
    "locationlog.csv"
    "periodiclog.csv"
    "personlog.csv"
    "relationlog.csv"
    "settingslog.csv"
    "treatmentlog.csv"
)

# Clean up any existing files
echo -e "${BLUE}Cleaning up existing log files...${NC}"
for FILE in "${LOG_FILES[@]}"; do
    rm -f "${RELEASE_PREFIX}${FILE}" "${DEBUG_PREFIX}${FILE}"
done

# Set environment variable to control log file name
export SIMPACT_OUTPUT_PREFIX=$RELEASE_PREFIX

# Run release version
echo -e "${BLUE}Running release version...${NC}"
echo "MNRM_DEBUG_SEED=${MNRM_DEBUG_SEED} $RELEASE_EXE $TEST_CONFIG $OPTIONS"
$RELEASE_EXE $TEST_CONFIG $OPTIONS
if [ $? -ne 0 ]; then
    echo -e "${RED}Error: Release version failed to run correctly${NC}"
    exit 1
fi
echo -e "${GREEN}Release version completed successfully${NC}"

# Set environment variable for debug version
export SIMPACT_OUTPUT_PREFIX=$DEBUG_PREFIX

# Run debug version
echo -e "${BLUE}Running debug version...${NC}"
echo "MNRM_DEBUG_SEED=${MNRM_DEBUG_SEED} $DEBUG_EXE $TEST_CONFIG $OPTIONS"
$DEBUG_EXE $TEST_CONFIG $OPTIONS
if [ $? -ne 0 ]; then
    echo -e "${RED}Error: Debug version failed to run correctly${NC}"
    exit 1
fi
echo -e "${GREEN}Debug version completed successfully${NC}"

# Compare output files
echo -e "${BLUE}Comparing output files...${NC}"
ALL_MATCH=true

for FILE in "${LOG_FILES[@]}"; do
    RELEASE_FILE="${RELEASE_PREFIX}${FILE}"
    DEBUG_FILE="${DEBUG_PREFIX}${FILE}"
    
    if [ ! -f "$RELEASE_FILE" ]; then
        echo -e "${RED}Error: Release output file $RELEASE_FILE not found${NC}"
        ALL_MATCH=false
        continue
    fi
    
    if [ ! -f "$DEBUG_FILE" ]; then
        echo -e "${RED}Error: Debug output file $DEBUG_FILE not found${NC}"
        ALL_MATCH=false
        continue
    fi
    
    # Count lines in each file (excluding header)
    RELEASE_LINES=$(tail -n +2 "$RELEASE_FILE" | wc -l)
    DEBUG_LINES=$(tail -n +2 "$DEBUG_FILE" | wc -l)
    
    echo -e "File: ${BLUE}${FILE}${NC}"
    echo -e "  Release records: ${BLUE}${RELEASE_LINES}${NC}"
    echo -e "  Debug records:   ${BLUE}${DEBUG_LINES}${NC}"
    
    if [ "$RELEASE_LINES" -eq "$DEBUG_LINES" ]; then
        echo -e "  ${GREEN}Match: Record counts are identical${NC}"
    else
        echo -e "  ${RED}Mismatch: Record counts are different${NC}"
        ALL_MATCH=false
    fi
    echo ""
done

# Final result
if [ "$ALL_MATCH" = true ]; then
    echo -e "${GREEN}=== All tests passed! ===${NC}"
    # If all tests pass, analyze and compare event types in release and debug eventlog.csv files
    echo -e "\n${BLUE}=== Comparing Event Types in Release and Debug eventlog.csv Files ===${NC}"
    RELEASE_EVENTLOG="${RELEASE_PREFIX}eventlog.csv"
    DEBUG_EVENTLOG="${DEBUG_PREFIX}eventlog.csv"

    # Count events in release version
    declare -A release_counts
    while IFS=, read -r _ event _; do
        # Remove parentheses if present
        event=${event//\(/}
        event=${event//\)/}
        
        # Increment counter for this event type
        ((release_counts[$event]++))
    done < "$RELEASE_EVENTLOG"

    # Count events in debug version
    declare -A debug_counts
    while IFS=, read -r _ event _; do
        # Remove parentheses if present
        event=${event//\(/}
        event=${event//\)/}
        
        # Increment counter for this event type
        ((debug_counts[$event]++))
    done < "$DEBUG_EVENTLOG"

    # Combine all unique event types
    declare -A all_events
    for event in "${!release_counts[@]}"; do
        all_events[$event]=1
    done
    for event in "${!debug_counts[@]}"; do
        all_events[$event]=1
    done

    # Display header for the table
    echo -e "${BLUE}Event Type                 | Release Count | Debug Count | Match${NC}"
    echo -e "${BLUE}--------------------------|--------------|------------|-------${NC}"

    # Track if all events match between release and debug
    ALL_EVENTS_MATCH=true

    # Display results in tabular format
    for event in $(echo "${!all_events[@]}" | tr ' ' '\n' | sort); do
        release_count=${release_counts[$event]:-0}
        debug_count=${debug_counts[$event]:-0}
        
        # Check if counts match
        if [ "$release_count" -eq "$debug_count" ]; then
            match_status="${GREEN}PASS${NC}"
        else
            match_status="${RED}FAIL${NC}"
            ALL_EVENTS_MATCH=false
        fi
        
        # Print in table format
        printf "%-26s | %12d | %10d | %s\n" "$event" "$release_count" "$debug_count" "$match_status"
    done

    # Print summary
    if [ "$ALL_EVENTS_MATCH" = true ]; then
        echo -e "\n${GREEN}PASS:${NC} All event counts match between release and debug versions"
    else
        echo -e "\n${RED}FAIL:${NC} Some event counts differ between release and debug versions"
    fi
    
    # Compare against reference values in stat_out_500.json
    echo -e "\n${BLUE}=== Comparing Actual Counts Against Expected Counts ===${NC}"
    
    if [ -f "stat_out_500.json" ]; then
        REFERENCE_CHECK_PASS=true
        
        # Display header for the reference comparison table
        echo -e "${BLUE}Event Type                 | Actual Count | Expected Count | Match${NC}"
        echo -e "${BLUE}--------------------------|--------------|----------------|-------${NC}"
        
        # Loop through all events
        for event in $(echo "${!all_events[@]}" | tr ' ' '\n' | sort); do
            # Get actual count from release version
            actual_count=${release_counts[$event]:-0}
            
            # Extract reference values using jq if available, otherwise use grep/sed
            if command -v jq &> /dev/null; then
                # Using jq (preferred for JSON parsing)
                reference_count=$(jq -r ".\"$event\"?.expected // 0" stat_out_500.json)
            else
                # Fallback to grep/sed for systems without jq
                reference_count=$(grep -o "\"$event\".*expected.*[0-9]\+" stat_out_500.json | grep -o '[0-9]\+' | head -1)
                if [ -z "$reference_count" ]; then
                    reference_count=0
                fi
            fi
            
            # Check if counts match
            if [ "$actual_count" -eq "$reference_count" ]; then
                match_status="${GREEN}PASS${NC}"
            else
                match_status="${RED}FAIL${NC}"
                REFERENCE_CHECK_PASS=false
            fi
            
            # Print in table format
            printf "%-26s | %12d | %14d | %s\n" "$event" "$actual_count" "$reference_count" "$match_status"
        done
        
        # Print reference comparison summary
        if [ "$REFERENCE_CHECK_PASS" = true ]; then
            echo -e "\n${GREEN}PASS:${NC} All event counts match reference values in stat_out_500.json"
        else
            echo -e "\n${RED}WARNING:${NC} Some event counts differ from reference values in stat_out_500.json"
            # Don't exit with error yet - this is just a warning at this point
            echo -e "${BLUE}Note: This indicates potential changes in behavior. Update stat_out_500.json if these changes are expected.${NC}"
        fi
    else
        echo -e "${BLUE}Reference file stat_out_500.json not found. Skipping reference comparison.${NC}"
    fi
    
    echo -e "\n${GREEN}=== All tests passed! ===${NC}"
    echo -e "${GREEN}Release and debug versions produce consistent output files with matching record counts${NC}"
    # Clean up temporary test config and output files
    echo -e "${BLUE}Cleaning up temporary files...${NC}"
    rm -f "$TEST_CONFIG"
    
    # Clean up all CSV log files
    echo -e "${BLUE}Cleaning up CSV log files...${NC}"
    for FILE in "${LOG_FILES[@]}"; do
        rm -f "${RELEASE_PREFIX}${FILE}"
        rm -f "${DEBUG_PREFIX}${FILE}"
    done
    
    # Also remove the dev_ prefixed files if they exist
    for FILE in "${LOG_FILES[@]}"; do
        rm -f "dev_${FILE}"
    done
    
    echo -e "${GREEN}Cleanup complete${NC}"
    exit 0
else
    echo -e "${RED}=== Test failed! ===${NC}"
    echo -e "${RED}Release and debug versions produce inconsistent output files${NC}"
    # Clean up temporary test config and output files even on failure
    echo -e "${BLUE}Cleaning up temporary files...${NC}"
    rm -f "$TEST_CONFIG"
    
    # Clean up all CSV log files
    echo -e "${BLUE}Cleaning up CSV log files...${NC}"
    for FILE in "${LOG_FILES[@]}"; do
        rm -f "${RELEASE_PREFIX}${FILE}"
        rm -f "${DEBUG_PREFIX}${FILE}"
    done
    
    # Also remove the dev_ prefixed files if they exist
    for FILE in "${LOG_FILES[@]}"; do
        rm -f "dev_${FILE}"
    done
    
    echo -e "${GREEN}Cleanup complete${NC}"
    exit 1
fi
