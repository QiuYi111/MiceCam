#!/usr/bin/env bash
# verify-ai.sh — Harness AI Compliance Verification
# Checks project files and AI anti-patterns for the MiceCam project.
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"

passed=0
failed=0
warnings=0

check_file() {
    local label="$1"
    local path="$2"
    local required="${3:-true}"

    if [ -f "$path" ]; then
        echo "✅ $label exists"
        passed=$((passed + 1))
    elif [ "$required" = "true" ]; then
        echo "❌ $label MISSING: $path"
        failed=$((failed + 1))
    else
        echo "⚠️  $label not found (optional): $path"
        warnings=$((warnings + 1))
    fi
}

check_no_pattern() {
    local label="$1"
    local pattern="$2"
    local dir="$3"
    local required="${4:-true}"

    local matches
    matches=$(grep -rl "$pattern" "$dir" --include="*.cpp" --include="*.h" --include="*.py" 2>/dev/null || true)

    if [ -z "$matches" ]; then
        echo "✅ $label: clean"
        passed=$((passed + 1))
    elif [ "$required" = "true" ]; then
        echo "❌ $label found in:"
        echo "$matches" | sed 's/^/   /'
        failed=$((failed + 1))
    else
        echo "⚠️  $label found in some files"
        warnings=$((warnings + 1))
    fi
}

echo "=== MiceCam AI Compliance Verification ==="
echo ""

# --- Required project files ---
echo "--- Required Project Files ---"
check_file "CONSTITUTION.md" "$PROJECT_ROOT/CONSTITUTION.md"
check_file "CONTEXT.md" "$PROJECT_ROOT/CONTEXT.md"
check_file "AGENTS.md" "$PROJECT_ROOT/AGENTS.md"
check_file "CLAUDE.md" "$PROJECT_ROOT/CLAUDE.md"
check_file "CACHE.md" "$PROJECT_ROOT/CACHE.md"
check_file ".harness/config.yaml" "$PROJECT_ROOT/.harness/config.yaml"
echo ""

# --- Spec directory check ---
echo "--- Spec Directory ---"
if [ -d "$PROJECT_ROOT/specs" ]; then
    count=$(find "$PROJECT_ROOT/specs" -name "spec.md" | wc -l)
    echo "✅ specs/ exists ($count spec(s) found)"
    passed=$((passed + 1))
else
    echo "⚠️  No specs/ directory found"
    warnings=$((warnings + 1))
fi
echo ""

# --- AI anti-pattern scan (source code) ---
echo "--- AI Anti-Pattern Scan ---"
check_no_pattern "TODO comments in code" "TODO" "$PROJECT_ROOT/internal" true
check_no_pattern "TODO comments in code" "TODO" "$PROJECT_ROOT/api" true
check_no_pattern "FIXME comments in code" "FIXME" "$PROJECT_ROOT/internal" false
echo ""

# --- Summary ---
echo "=== Summary ==="
echo "✅ $passed passed"
echo "❌ $failed failed"
echo "⚠️  $warnings warnings"
echo ""

if [ "$failed" -eq 0 ]; then
    echo "🎉 All required checks passed."
    exit 0
else
    echo "🚨 $failed required check(s) failed. Fix before merging."
    exit 1
fi
