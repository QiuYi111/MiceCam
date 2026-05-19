# Stage Definitions

## product_definition ✅ (complete)

Goal: Define what to build, for whom, and why it matters.

Exit criteria met: product.md, evidence (grill), user-journeys, ui-direction, value-proposition, roadmap, state all complete.

---

## feasibility (active)

Goal: Validate that the two riskiest technical assumptions are viable:
1. OAK-D H264 hardware encoding via DepthAI VideoEncoder(H264) node
2. Cross-platform FFmpeg hardware encoder selection and fallback chain

Allowed: technical spikes, prototyping, proof-of-concept experiments, dependency evaluation

Forbidden: production implementation, schema changes, deployment changes

Exit criteria:
- spike-report.md with recommendation: continue
- feasibility_ready: true

---

## delivery

Goal: Implement bounded tasks that advance the current roadmap stage.

Allowed: feature implementation within task scope, test writing, debugging within task scope, documentation updates

Forbidden: changing product positioning, expanding MVP boundary, changing core tech stack, core/infra risk changes without user approval

Exit criteria: task acceptance criteria met, worker-report.md, tests pass
