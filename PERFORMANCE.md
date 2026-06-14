# Performance Log

A running record of performance work on the matching engine: what was changed, why,
and the measured effect. Newest entries go at the top.

## Methodology

All numbers come from `./build/orderbook_bench` unless noted otherwise.

- Workload: 500,000 mixed operations across 200 symbols (60% limit / 30% cancel·reduce
  / 10% market; ~80% of traffic concentrated in ~50 "hot" symbols).
- Each operation is timed individually with the hardware cycle counter
  (`rdtsc` on x86-64, `cntvct_el0` on arm64); timer overhead is measured and subtracted.
- Reported percentiles are averaged over 10 runs.
- Build: `-O3 -march=native -DNDEBUG`.

**Always record the machine** — these figures are hardware-dependent and not comparable
across CPUs. Re-measure a clean baseline whenever you switch machines.

### Entry template

Copy this block for each change.

```
## YYYY-MM-DD — <short title>

**Change:** what was modified (files / data structures / algorithm).
**Rationale:** why this should help (hypothesis, profiling evidence).
**Machine:** CPU, clock, OS, compiler.

| Metric        | Before    | After     | Δ        |
| ------------- | --------- | --------- | -------- |
| P90 latency   |           |           |          |
| P99 latency   |           |           |          |
| P99.9 latency |           |           |          |
| Cycles per op |           |           |          |

**Result:** did it help? Any surprises / regressions / follow-ups.
```

---

## 2026-06-13 — Baseline

**Change:** none — initial reference measurement.
**Machine:** AMD Ryzen 7 PRO 8700GE (x86-64), TSC ≈ 3.65 GHz (3.650 ticks/ns, measured timer overhead 36 ticks).

| Metric        | Value     |
| ------------- | --------- |
| P90 latency   | 142.2 ns  |
| P99 latency   | 274.1 ns  |
| P99.9 latency | 963.1 ns  |
| Cycles per op | 276.6     |

**Result:** baseline for all future comparisons.

### Ideas to explore

- Replace per-level `std::list<unique_ptr<LimitOrder>>` allocations with a pooled/arena
  allocator to cut per-order heap churn.
- Store orders by value (or in a flat slab) instead of `unique_ptr` to improve cache
  locality when walking a price level.
- Consider a flat array / intrusive structure for price levels in place of `std::map`
  to reduce node-chasing on the hot path.
- Reserve/pre-size the per-book `unordered_map<OrderID, LookUp>` and the engine's
  `idToSymbol` map to avoid rehashing under load.
