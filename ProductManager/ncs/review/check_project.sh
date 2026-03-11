#!/bin/bash

#
# NCS Project Quick Check Script
# 
# This script performs automated checks on an NCS project structure
# and reports basic compliance issues.
#
# Usage: ./check_project.sh [project_directory]
#

set -e

# Colors for output
RED='\033[0;31m'
YELLOW='\033[1;33m'
GREEN='\033[0;32m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Counters
CRITICAL=0
WARNING=0
PASS=0

# Project directory
PROJECT_DIR="${1:-.}"

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}NCS Project Quick Check${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""
echo "Project directory: $PROJECT_DIR"
echo ""

# Function to print results
print_pass() {
    echo -e "${GREEN}✅ PASS${NC}: $1"
    ((PASS++))
}

print_warning() {
    echo -e "${YELLOW}⚠️  WARN${NC}: $1"
    ((WARNING++))
}

print_critical() {
    echo -e "${RED}❌ FAIL${NC}: $1"
    ((CRITICAL++))
}

# Check if directory exists
if [ ! -d "$PROJECT_DIR" ]; then
    print_critical "Project directory does not exist: $PROJECT_DIR"
    exit 1
fi

cd "$PROJECT_DIR"

echo -e "${BLUE}1. Required Files Check${NC}"
echo "-----------------------------------"

# Required files
required_files=("CMakeLists.txt" "Kconfig" "prj.conf" "src/main.c" "README.md" "LICENSE")

for file in "${required_files[@]}"; do
    if [ -f "$file" ]; then
        print_pass "Found $file"
    else
        print_critical "Missing $file"
    fi
done

echo ""
echo -e "${BLUE}2. .gitignore Check${NC}"
echo "-----------------------------------"

if [ -f ".gitignore" ]; then
    if grep -q "build/" ".gitignore"; then
        print_pass ".gitignore includes build directories"
    else
        print_warning ".gitignore should include 'build/'"
    fi
    
    if grep -q "*.log" ".gitignore"; then
        print_pass ".gitignore includes log files"
    else
        print_warning ".gitignore should include '*.log'"
    fi
else
    print_critical "Missing .gitignore file"
fi

echo ""
echo -e "${BLUE}3. Copyright Headers Check${NC}"
echo "-----------------------------------"

# Check C source files for copyright. Use CURRENT YEAR (e.g. 2026) for new or
# substantially modified files; keep original year for unmodified upstream files.
c_files_count=0
c_files_with_copyright=0
current_year=$(date +%Y)
files_old_year=""

if [ -d "src" ]; then
    while IFS= read -r -d '' file; do
        ((c_files_count++))
        if head -n 10 "$file" | grep -q "Copyright"; then
            ((c_files_with_copyright++))
            # Warn if copyright year is not current (e.g. 2025 when current is 2026)
            year_in_file=$(head -n 10 "$file" | grep -oE "Copyright \(c\) [0-9]{4}" | head -1 | grep -oE "[0-9]{4}")
            if [ -n "$year_in_file" ] && [ "$year_in_file" -lt "$current_year" ] 2>/dev/null; then
                files_old_year="${files_old_year}${file} (${year_in_file})\n"
            fi
        fi
    done < <(find src -type f \( -name "*.c" -o -name "*.h" \) -print0)
    
    if [ $c_files_count -eq $c_files_with_copyright ]; then
        print_pass "All $c_files_count source files have copyright headers"
    elif [ $c_files_with_copyright -gt 0 ]; then
        print_warning "$c_files_with_copyright of $c_files_count files have copyright headers"
    else
        print_critical "No copyright headers found in source files"
    fi
    if [ -n "$files_old_year" ]; then
        print_warning "Some source files have copyright year before $current_year; use current year for new or substantially modified files"
    fi
else
    print_critical "src/ directory not found"
fi

echo ""
echo -e "${BLUE}4. CMakeLists.txt Validation${NC}"
echo "-----------------------------------"

if [ -f "CMakeLists.txt" ]; then
    if grep -q "cmake_minimum_required" "CMakeLists.txt"; then
        print_pass "CMakeLists.txt has minimum version"
    else
        print_warning "CMakeLists.txt missing cmake_minimum_required"
    fi
    
    if grep -q "find_package(Zephyr" "CMakeLists.txt"; then
        print_pass "CMakeLists.txt finds Zephyr package"
    else
        print_critical "CMakeLists.txt missing find_package(Zephyr)"
    fi
    
    if grep -q "project(" "CMakeLists.txt"; then
        print_pass "CMakeLists.txt defines project"
    else
        print_critical "CMakeLists.txt missing project() definition"
    fi
    
    if grep -q "target_sources" "CMakeLists.txt"; then
        print_pass "CMakeLists.txt has source files"
    else
        print_warning "CMakeLists.txt may be missing source files"
    fi
fi

echo ""
echo -e "${BLUE}5. Kconfig Check${NC}"
echo "-----------------------------------"

if [ -f "Kconfig" ]; then
    if grep -q 'source "Kconfig.zephyr"' "Kconfig"; then
        print_pass "Kconfig sources Kconfig.zephyr"
    else
        print_critical "Kconfig missing 'source \"Kconfig.zephyr\"'"
    fi
    
    if grep -q "menu\|config" "Kconfig"; then
        print_pass "Kconfig defines configuration options"
    else
        print_warning "Kconfig may not have any configuration options"
    fi
fi

echo ""
echo -e "${BLUE}6. Configuration Files${NC}"
echo "-----------------------------------"

if [ -f "prj.conf" ]; then
    # Check for common Wi-Fi configurations
    if grep -q "CONFIG_WIFI=y" "prj.conf"; then
        print_pass "Wi-Fi enabled in prj.conf"
        
        # Check Wi-Fi memory configuration
        heap_size=$(grep "CONFIG_HEAP_MEM_POOL_SIZE" "prj.conf" | grep -o '[0-9]*' || echo "0")
        if [ "$heap_size" -ge 80000 ]; then
            print_pass "Heap size adequate for Wi-Fi ($heap_size bytes)"
        elif [ "$heap_size" -gt 0 ]; then
            print_warning "Heap size may be too small for Wi-Fi ($heap_size < 80000)"
        fi
    fi
    
    # Check for logging
    if grep -q "CONFIG_LOG=y" "prj.conf"; then
        print_pass "Logging enabled"
    else
        print_warning "Logging not enabled (recommended for development)"
    fi
fi

# Check overlay files
overlay_count=$(find . -maxdepth 1 -name "overlay-*.conf" | wc -l)
if [ "$overlay_count" -gt 0 ]; then
    print_pass "Found $overlay_count overlay file(s)"
else
    print_warning "No overlay files found (may be appropriate)"
fi

echo ""
echo -e "${BLUE}7. README.md Check${NC}"
echo "-----------------------------------"

if [ -f "README.md" ]; then
    # Check for required sections
    required_sections=("Overview" "Hardware" "Build" "Quick Start")
    
    for section in "${required_sections[@]}"; do
        if grep -qi "$section" "README.md"; then
            print_pass "README has $section section"
        else
            print_warning "README missing $section section"
        fi
    done
    
    # Check for build commands
    if grep -q "west build" "README.md"; then
        print_pass "README includes build commands"
    else
        print_warning "README should include build commands"
    fi
fi

echo ""
echo -e "${BLUE}8. Security Check${NC}"
echo "-----------------------------------"

# Check for hardcoded credentials
if grep -r "password\|PASSWORD" --include="*.c" --include="*.h" --include="*.conf" . 2>/dev/null | grep -v ".git" | grep -q "="; then
    print_critical "Potential hardcoded credentials found!"
else
    print_pass "No obvious hardcoded credentials detected"
fi

# Check if overlay files with credentials are in .gitignore
if [ -f ".gitignore" ] && [ -f "overlay-wifi-static.conf" ]; then
    if grep -q "overlay-wifi-static.conf" ".gitignore"; then
        print_pass "Static credential overlay in .gitignore"
    else
        print_warning "Consider adding credential overlays to .gitignore"
    fi
fi

echo ""
echo -e "${BLUE}9. Build Artifacts Check${NC}"
echo "-----------------------------------"

# Check if build directory is committed
if [ -d ".git" ]; then
    if git ls-files --error-unmatch build/ 2>/dev/null; then
        print_critical "build/ directory is tracked by git"
    else
        print_pass "build/ directory not in git (good)"
    fi
    
    # Check for other problematic files
    if git ls-files | grep -q "\.bin$\|\.hex$\|\.elf$"; then
        print_warning "Binary files found in git repository"
    else
        print_pass "No binary files in git repository"
    fi
fi

echo ""
echo -e "${BLUE}10. Optional Files${NC}"
echo "-----------------------------------"

optional_files=("west.yml" "sysbuild.conf" "VERSION" "boards/")

for file in "${optional_files[@]}"; do
    if [ -e "$file" ]; then
        print_pass "Found $file"
    fi
done

echo ""
echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Summary${NC}"
echo -e "${BLUE}========================================${NC}"
echo ""
echo -e "${GREEN}Passed:   $PASS${NC}"
echo -e "${YELLOW}Warnings: $WARNING${NC}"
echo -e "${RED}Critical: $CRITICAL${NC}"
echo ""

# Determine overall status
if [ $CRITICAL -eq 0 ] && [ $WARNING -eq 0 ]; then
    echo -e "${GREEN}✅ Project structure looks good!${NC}"
    exit 0
elif [ $CRITICAL -eq 0 ]; then
    echo -e "${YELLOW}⚠️  Project has some warnings. Review recommended.${NC}"
    exit 0
else
    echo -e "${RED}❌ Project has critical issues. Please address them.${NC}"
    exit 1
fi
