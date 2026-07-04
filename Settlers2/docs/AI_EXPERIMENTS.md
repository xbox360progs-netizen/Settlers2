# AI Experiment Log

Each entry documents a hypothesis-driven change to SettlementSystem,
with quantitative evaluation criteria defined before the run.

## Template

```
Experiment AI-NNN
==================

Hypothesis
----------
(What behaviour change is expected and why.)

Change
------
(Which file/rule/method was modified and how.)

Control
-------
Seed:
Ticks:
Configuration:

Primary Metrics
---------------
Average CV:
Oscillating resources:
Propagation chains:
Cascade depth:

Guard Metrics
-------------
Flow <= Potential:
Crisis count:
Soldier production:
Contract tests:

Result
------
Confirmed / Partially confirmed / Rejected

(Quantitative delta when available.)
```

---

## Experiment AI-001 — CoalMine Expansion with Hysteresis

**Status: Pending** (requires Xbox 360 SDK for execution)

### Hypothesis

CoalMine expansion after sustained saturation (3 consecutive windows
of full capacity + depleted buffer) will reduce instability propagation
through the metallurgy chain (Coal → IronBar → Weapons → Soldiers)
without increasing overall crisis count.

### Change

- `SettlementSystem::BootstrapMetallurgy` — Rule 5 (new)
- `SettlementSystem.h` — `ExpansionSignal` struct, `m_coalSignal` member
- Decision logic: expand only when `flow ≈ potential && buffer ≤ 2`
  persists for 3 full windows (≈3000 ticks). Signal resets during
  construction to prevent cascade builds.

### Control

| Parameter | Value |
|---|---|
| Seed | Fixed (deterministic) |
| Ticks | 500000 |
| Scenario | T53FullEconomySoak |
| Economy config | All industries enabled, SettlementSystem active |
| Baseline | Rule 5 disabled (`#if 0`) |
| Modified | Rule 5 active |

### Expected Effect

| Metric | Baseline (estimated) | Target |
|---|---|---|
| Coal CV | ~45–50% (oscillating) | < 30% (stable) |
| Coal Streak | ~8+ windows | < 3 windows |
| Cascade depth | 3–4 (Coal → ... → Soldiers) | ≤ 2 |
| Oscillating resources | ~5–7 | ≤ 4 |

### Guard Metrics (must not regress)

| Metric | Criterion |
|---|---|
| Flow ≤ Potential | Must hold for all resources |
| Crisis count | Must not increase |
| Soldier production | Must not decrease |
| Contract tests (T46–T54) | All pass |

### Interpretation Rules

- **Confirmed**: all primary metrics improve, no guard metric regresses
- **Partially confirmed**: primary metrics improve but one guard metric
  shows regression (trade-off documented)
- **Rejected**: no measurable improvement or multiple guard failures
