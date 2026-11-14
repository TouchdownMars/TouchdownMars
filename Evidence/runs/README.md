# Run records: same seed, one change

Two flights of the First Crew objective in the Advanced model on seed 4242, flown on 14 November 2026 by the development build's automatic test run (`-TDSurface -TDOpsDemo -TDSeed=4242`). The second flight changes one design pick: Mastcam-Z replaced by SuperCam (`-TDPick=MastcamZ:0,SuperCam:1`).

Compare `event_list_hash` and `event_ids` (equal: the same events in the same order), then `design_picks`, `design_budget`, `phases` (one entry per phase: cells, demand, levels after, cause key, tasks finished, crew dose) and `score`. The model steps by phase, not by sol; data volumes are design-time estimates.
