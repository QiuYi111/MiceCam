# CACHE.md

This repository is optimized for cache-friendly agentic coding.

## Context Order

1. Harness skill instructions
2. AGENTS.md
3. CACHE.md
4. .harness/policies/blast-radius.yaml
5. .harness/policies/roles.yaml
6. project_index/repo.md
7. project_index/modules/*.md
8. active specs/<feature>/spec.md
9. active specs/<feature>/plan.md
10. active specs/<feature>/tasks.md
11. dynamic diff / logs / user request

## Rules

- Stable content goes first.
- Dynamic content goes last.
- Sort file lists lexicographically.
- Do not put timestamps in stable files.
- Do not rewrite stable policy files unless necessary.
- Append ADRs instead of rewriting architecture history.
- Ignore generated folders: node_modules, dist, build, .git, coverage.
