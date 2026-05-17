#!/usr/bin/env bash
# classify-risk.sh — Path-based blast radius classifier for Harness v2
# Outputs: leaf, branch, core, or infra
set -euo pipefail

risk_priority() {
    case "$1" in
        infra) echo 4 ;;
        core)  echo 3 ;;
        branch) echo 2 ;;
        leaf)  echo 1 ;;
    esac
}

max_risk() {
    local current="$1"
    local new="$2"
    if [ "$(risk_priority "$new")" -gt "$(risk_priority "$current")" ]; then
        echo "$new"
    else
        echo "$current"
    fi
}

if git rev-parse --git-dir > /dev/null 2>&1; then
    changed_files=$(git diff --name-only HEAD 2>/dev/null || git diff --name-only 2>/dev/null)
else
    changed_files=$(git diff --name-only 2>/dev/null)
fi

if [ -z "$changed_files" ]; then
    echo "No changed files detected."
    echo "Overall risk: leaf"
    exit 0
fi

echo "Analyzing changed files for blast radius..."
echo ""

risk="leaf"

for file in $changed_files; do
    file_risk="branch"

    case "$file" in
        .github/*|Dockerfile*|docker-compose*|*.env*|.gitlab-ci.yml|Jenkinsfile|terraform/*|CMakeLists.txt|vcpkg.json)
            file_risk="infra"
            ;;
        internal/domain/*|*auth*|*permission*|*rbac*)
            file_risk="core"
            ;;
        api/*|cmd/*|bindings/*)
            file_risk="branch"
            ;;
        docs/*|*.md|tests/*|scripts/*|templates/*|*.txt|maintenance/*)
            file_risk="leaf"
            ;;
        *)
            file_risk="branch"
            ;;
    esac

    risk=$(max_risk "$risk" "$file_risk")
    echo "  $file → $file_risk"
done

echo ""
echo "Overall risk: $risk"

echo ""
echo "Required gates:"
case "$risk" in
    leaf)
        echo "  - lint"
        echo "  - unit_test"
        ;;
    branch)
        echo "  - spec"
        echo "  - plan"
        echo "  - tests"
        echo "  - review_agent"
        ;;
    core)
        echo "  - human_spec_review"
        echo "  - architecture_review"
        echo "  - rollback_plan"
        echo "  - security_review"
        ;;
    infra)
        echo "  - dry_run"
        echo "  - explicit_human_approval"
        echo "  - rollback_plan"
        echo "  - security_review"
        ;;
esac
