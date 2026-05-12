# Graph Report - minnow  (2026-05-11)

## Corpus Check
- 25 files · ~9,912 words
- Verdict: corpus is large enough that graph structure adds value.

## Summary
- 113 nodes · 139 edges · 22 communities (19 shown, 3 thin omitted)
- Extraction: 82% EXTRACTED · 18% INFERRED · 0% AMBIGUOUS · INFERRED: 25 edges (avg confidence: 0.8)
- Token cost: 0 input · 0 output

## Graph Freshness
- Built from commit: `9462423a`
- Run `git rev-parse HEAD` and compare to check if the graph is stale.
- Run `graphify update .` after code changes (no API cost).

## Community Hubs (Navigation)
- [[_COMMUNITY_Community 0|Community 0]]
- [[_COMMUNITY_Community 2|Community 2]]
- [[_COMMUNITY_Community 3|Community 3]]
- [[_COMMUNITY_Community 4|Community 4]]
- [[_COMMUNITY_Community 5|Community 5]]
- [[_COMMUNITY_Community 8|Community 8]]
- [[_COMMUNITY_Community 9|Community 9]]
- [[_COMMUNITY_Community 10|Community 10]]
- [[_COMMUNITY_Community 11|Community 11]]
- [[_COMMUNITY_Community 12|Community 12]]

## God Nodes (most connected - your core abstractions)
1. `CheckSystemCall()` - 19 edges
2. `to_string()` - 8 edges
3. `getsockopt()` - 5 edges
4. `setsockopt()` - 5 edges
5. `with_color()` - 4 edges
6. `get_address()` - 4 edges
7. `FDWrapper::FDWrapper()` - 4 edges
8. `wait_next_event()` - 4 edges
9. `stress_test()` - 3 edges
10. `program_body()` - 3 edges

## Surprising Connections (you probably didn't know these)
- `Timer::Timer()` --calls--> `CheckSystemCall()`  [INFERRED]
  tests/common.cc → util/file_descriptor.cc
- `Timeout()` --calls--> `CheckSystemCall()`  [INFERRED]
  tests/common.cc → util/file_descriptor.cc
- `with_color()` --calls--> `to_string()`  [INFERRED]
  tests/common.cc → util/address.cc
- `speed_test()` --calls--> `to_string()`  [INFERRED]
  tests/byte_stream_speed_test.cc → util/address.cc
- `stress_test()` --calls--> `to_string()`  [INFERRED]
  tests/byte_stream_stress_test.cc → util/address.cc

## Communities (22 total, 3 thin omitted)

### Community 0 - "Community 0"
Cohesion: 0.23
Nodes (16): CheckSystemCall(), accept(), bind(), bind_to_device(), connect(), get_address(), listen(), local_address() (+8 more)

### Community 2 - "Community 2"
Cohesion: 0.22
Nodes (3): Address(), gai_error_category, ip_port()

### Community 3 - "Community 3"
Cohesion: 0.2
Nodes (6): close(), FDWrapper::close(), FDWrapper::FDWrapper(), FileDescriptor(), set_blocking(), write()

### Community 4 - "Community 4"
Cohesion: 0.31
Nodes (5): diagnostic(), print_debug_messages(), Timeout(), Timer::Timer(), with_color()

### Community 5 - "Community 5"
Cohesion: 0.31
Nodes (8): main(), program_body(), stress_test(), to_string(), wait_next_event(), getsockopt(), Socket(), throw_if_error()

### Community 8 - "Community 8"
Cohesion: 0.5
Nodes (3): bidirectional_stream_copy(), main(), show_usage()

### Community 9 - "Community 9"
Cohesion: 0.83
Nodes (3): main(), program_body(), speed_test()

## Knowledge Gaps
- **3 thin communities (<3 nodes) omitted from report** — run `graphify query` to explore isolated nodes.

## Suggested Questions
_Questions this graph is uniquely positioned to answer:_

- **Why does `to_string()` connect `Community 5` to `Community 9`, `Community 2`, `Community 3`, `Community 4`?**
  _High betweenness centrality (0.227) - this node is a cross-community bridge._
- **Why does `CheckSystemCall()` connect `Community 0` to `Community 3`, `Community 4`, `Community 5`?**
  _High betweenness centrality (0.184) - this node is a cross-community bridge._
- **Why does `FDWrapper::FDWrapper()` connect `Community 3` to `Community 0`, `Community 5`?**
  _High betweenness centrality (0.165) - this node is a cross-community bridge._
- **Are the 14 inferred relationships involving `CheckSystemCall()` (e.g. with `Timer::Timer()` and `Timeout()`) actually correct?**
  _`CheckSystemCall()` has 14 INFERRED edges - model-reasoned connections that need verification._
- **Are the 6 inferred relationships involving `to_string()` (e.g. with `stress_test()` and `with_color()`) actually correct?**
  _`to_string()` has 6 INFERRED edges - model-reasoned connections that need verification._
- **Are the 2 inferred relationships involving `getsockopt()` (e.g. with `CheckSystemCall()` and `wait_next_event()`) actually correct?**
  _`getsockopt()` has 2 INFERRED edges - model-reasoned connections that need verification._