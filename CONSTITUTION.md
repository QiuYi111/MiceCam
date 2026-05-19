# Constitution: MiceCam

> This document defines the rules, principles, and governance structure for the project. All contributors and AI agents must follow these rules. Changes to this document follow the Amendment Process below.

## Purpose

This constitution exists so that every contributor, reviewer, and AI agent shares the same understanding of how this project is built and maintained. It removes ambiguity from technical decisions, establishes clear ownership, and ensures consistency across codebases and teams.

## Core Principles

1. **Contract-First Development**: All integration boundaries must have a written contract (API spec, event schema, or interface definition) before implementation begins. Code that crosses a service or module boundary without a contract is not accepted.

2. **Strict DDD Layer Isolation**: The codebase follows a layered architecture (domain, app, infra, interface). Each layer has explicit rules about what it can depend on. Domain entities never reference infrastructure. Infrastructure never contains business logic. Violations are caught by static analysis and code review.

3. **Test-Driven Development**: Features are implemented using the red-green-refactor cycle. Tests are written before production code. Tests are not modified during the green phase (once a test passes, only production code changes). Test coverage thresholds are enforced in CI.

4. **Observability as Requirement**: Logging, metrics, and tracing are not afterthoughts. Every external-facing operation must emit structured logs with correlation IDs. Key business operations must have metrics. Failure paths must produce actionable error messages.

5. **Security by Default**: Input validation on every external boundary. No secrets in code or logs. Dependencies are scanned for known vulnerabilities. Authentication and authorization are enforced at the infrastructure layer, not left to individual handlers.

6. **Small, Reviewable Changes**: Pull requests should be small enough to review in a single sitting. Large features are broken into incremental steps, each independently reviewable and deployable. If a PR touches more than 400 lines of production code, reconsider the breakdown.

7. **Documentation as Code**: Specs, plans, contracts, and reports live alongside the code in version control. They are updated in the same PR as the code they describe. Out-of-date documentation is treated as a bug.

8. **Blast Radius Classification**: Every change is classified by its blast radius (leaf, branch, core, infra) before implementation. Higher-radius changes require additional review and verification steps. Classification is recorded in the feature spec.

## Governance Rules

### Who Approves What

| Change Type | Approver | Review Requirement |
|-------------|----------|--------------------|
| leaf (docs, tests, isolated scripts) | Any reviewer | Light review, focus on accuracy |
| branch (features, services, endpoints) | Tech lead + 1 reviewer | Standard review |
| core (domain model, auth, permissions) | Tech lead + 2 reviewers | Extended review, must include domain expert |
| infra (deployment, CI/CD, secrets, migrations) | Tech lead + 2 reviewers | Extended review, must include infra owner |
| Constitution changes | Project lead + team consensus | Full team discussion (see Amendment Process) |

### Review Requirements

- All PRs require at least one approving review before merge.
- Domain and infrastructure changes require two approvals.
- Reviews must address correctness, test coverage, and blast radius classification.
- Reviewers must run `make verify` locally before approving.

### Merge Requirements

- All CI checks pass (tests, lint, static analysis).
- `make verify` exits clean.
- `make verify-ai` exits clean.
- Risk classification is recorded in the feature spec.
- Implementation report is filed for the feature.
- No open blocking issues in the review.

## Decision Framework

Technical decisions follow this process:

1. **Proposal**: Document the decision, alternatives considered, and trade-offs in a short ADR (Architecture Decision Record).
2. **Discussion**: Share with the team. Allow at least 24 hours for async feedback (or discuss in the next sync).
3. **Decision**: The tech lead makes the final call, incorporating team input. Record the decision and rationale.
4. **Implementation**: Update relevant contracts, specs, and code. Reference the ADR in commit messages.
5. **Review**: Verify the implementation matches the decision during code review.

For reversible decisions, prefer speed over deliberation. For irreversible decisions (schema changes, API contracts, dependency swaps), prefer thoroughness.

## Amendment Process

This constitution can be changed, but changes are deliberate:

1. **Proposal**: Open an issue describing the proposed change and its rationale.
2. **Discussion**: Team discusses the impact on existing workflows and code.
3. **Vote**: Simple majority of active contributors. Constitution changes are not merged on a single person's authority.
4. **Update**: Merge the change. Update `CLAUDE.md` and any agent instructions that reference the changed rules.
5. **Announce**: Notify all contributors of the change and what it means for their workflow.

## Enforcement

Rules are verified automatically where possible:

- **`make verify`**: Runs linters, type checks, and static analysis. Catches layer violations, missing tests, and code quality issues.
- **`make verify-ai`**: Scans for AI anti-patterns (hardcoded values, missing error handling, debug logging in production code, inconsistent naming).
- **CI pipeline**: Enforces test coverage thresholds, runs contract tests, and blocks merge on verification failures.
- **Code review**: Human reviewers check blast radius classification, documentation completeness, and architectural alignment.

When a rule is violated, the fix is blocking. The PR does not merge until the violation is resolved or the rule is formally amended.
