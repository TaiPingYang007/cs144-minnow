# PROJECT_CONTEXT - cs144

## 项目定位

- 正式仓库：`/home/taipingyang007/projects/cpp_project/05_Stanford-CS144-Computer_Network_Systems/cs144-minnow`
- GitHub 正式仓库：`TaiPingYang007/cs144-minnow`
- 定位：使用单个正式仓库持续推进 CS144 labs，不再依赖多仓库或 starter 分支路线

## 当前进度

- check0 ByteStream：已完成
- check1 Reassembler：已完成，官方测试 18/18
- check2 TCPReceiver：已完成，官方测试 30/30
- check3 TCPSender：已完成，本地 check3 测试 37/37（2026-06-20）；笔记见 `notes/check3-tcp-sender.md`
- 下一站：check4 NetworkInterface（ARP / IP↔MAC / 以太网帧）

当前学习入口仍是 **check4: NetworkInterface**。✅ 后续 starter 框架已补齐到可学习状态（2026-06-20）：check4 `NetworkInterface` 和本地 check5 `Router` 都已恢复到原始目录；check7/capstone 相关 app/support 也已恢复并可编译。`NetworkInterface` 与 `Router` 都保持 unimplemented 桩，没有混入解答。验证结果：`make check3` 回归全绿；`make check4` 运行到 `net_interface` 后因 `unimplemented send_datagram` 正确失败；`make check5` 编译通过后被前置 `net_interface` 未实现拦住。

## 关键文件

### 已完成模块

- check0 ByteStream
  - `src/byte_stream.hh`
  - `src/byte_stream.cc`
- check1 Reassembler
  - `src/reassembler.hh`
  - `src/reassembler.cc`
  - `tests/reassembler_single.cc`
  - `tests/reassembler_seq.cc`
  - `tests/reassembler_holes.cc`
- check2 TCPReceiver
  - `src/wrapping_integers.hh`
  - `src/wrapping_integers.cc`
  - `src/tcp_receiver.hh`
  - `src/tcp_receiver.cc`

### 后续重点模块

- check3 TCPSender
  - `src/tcp_sender.hh`
  - `src/tcp_sender.cc`
  - `tests/send_*.cc`
- check4 NetworkInterface
  - `src/network_interface.hh`
  - `src/network_interface.cc`
  - `tests/net_interface.cc`
  - `tests/network_interface_test_harness.hh`
- check5 Router（本地 `etc/tests.cmake` 的 check5）
  - `src/router.hh`
  - `src/router.cc`
  - `tests/router.cc`
- capstone/app support
  - `apps/tcp_ipv4.cc`
  - `apps/ip_raw.cc`
  - `apps/tcp_eth_udp.cc`
  - `apps/fun_router.cc`
  - `src/tcp_minnow_socket.cc`
  - `util/tcp_*` / `util/tun*` / `util/*fd_adapter*` 支撑文件

### 学习笔记

- check0：相关记录见项目记忆
- check1：相关记录见项目记忆
- check2：`notes/check2-tcp-receiver.md`

### 课程材料

- `handouts/check0.pdf` 到 `handouts/check7.pdf`

## 恢复入口

### 如果继续学 check3

1. 先读 `handouts/check3.pdf`。
2. 再读 `src/tcp_sender.hh`，先理解接口和成员变量要求。
3. 再读 `tests/send_*.cc`，用测试反推 Sender 要满足的行为。
4. 最后再动 `src/tcp_sender.cc`。

建议延续 check2 的节奏：

- 先建立直觉和全景。
- 再分阶段做理解检查。
- 最后写代码。
- 每完成一阶段，把笔记整理成“复习时从上到下顺读”的结构。

### 如果复习 check2

主入口：`notes/check2-tcp-receiver.md`

复习顺序：

1. 先看全貌：TCPReceiver 是“大门 + 翻译官 + 回执员”。
2. 再看反复卡点：wrap/unwrap、三套编号、SYN/FIN、ackno、window。
3. 做每阶段自测题。
4. 最后对照代码位置：
   - `src/wrapping_integers.cc`
   - `src/tcp_receiver.hh`
   - `src/tcp_receiver.cc`

## 稳定约束

- 只维护一个正式仓库，不再纠结 starter 分支路线。
- 远端当前只有 `main` 分支，后续 labs 不存在可直接切换的 starter 分支。
- 业务 `.cc` 文件保留自己实现，不把别人的完整答案原样带入正式仓库。
- 推进新 check 时优先保证理解，不为赶进度跳过全景和自测。

## 用户学习偏好

- 网络基础从接近零起点出发，新名词要先给直觉和场景，再给术语。
- 笔记要适合复习：从上到下顺读，突出关键点，不要写成流水账。
- 用户反复卡住的地方要醒目标出，并配自测题。
- 必要时指出代码位置，方便从概念跳回实现。
- 讲解时可以结合 check0 ByteStream、check1 Reassembler、check2 TCPReceiver 的已完成经验。

## 最近重要结论

- check2 已于 2026-06-13 完成，官方 check2 30/30。
- check2 的核心不是 wrap/unwrap 本身，而是 `receive()` 和 `send()` 两条路径。
- 三套编号是 check2 的地基：
  - seqno：网络上的 32 位序号，包含 SYN/FIN。
  - absolute seqno：Receiver 内部 64 位中转坐标，包含 SYN/FIN。
  - stream index：Reassembler 使用的 payload 索引，不包含 SYN/FIN。
- ackno 表示连续拼好的进度，不表示“见过的最大 seqno”。
- FIN 是否被确认要看 ByteStream 是否 `is_closed()`，不能只看当前报文是否带 FIN。
- window_size 要 `min(available_capacity, UINT16_MAX)`，不能直接截断成 `uint16_t`。

## Checkpoint starter 文件导入策略

当前 CS144 Minnow 仓库的 check0-check2 已完成并通过；进入 check3 时发现本地缺少 check3 所需的 starter/test 文件。本次没有 clone 或整体导入外部完整仓库，而是只从近官方公共镜像 `Richard-Qin-X/minnow` 的精确历史提交中提取 check3 starter 相关文件：

- 来源仓库：`Richard-Qin-X/minnow`
- 来源提交：`b4fae30bec030c40de5389a9576d5c782279626c`
- 提交信息：`CS144 Lab checkpoint 3`
- 作者：Keith Winstein
- 本地处理：仅导入缺失的 check3 starter/test 文件，并做最小必要的本地兼容性编辑；不导入任何解答实现或后续 HEAD 状态。

check4 恢复记录（2026-06-20）：

- 来源仓库：`Richard-Qin-X/minnow`
- 来源提交：`da1014dcfe27a49d70a2f3c6866d52b436d48f22`
- 提交信息：`CS144 Lab checkpoint 5`
- 作者：Keith Winstein
- 关键判断：该镜像历史里没有单独的 `CS144 Lab checkpoint 4` 提交；`net_interface` 测试在 check3 树里已存在但未注册，`da1014d` 新增 `src/network_interface.{hh,cc}` 并注册 `net_interface`。本地 `etc/tests.cmake` 中 `check4` 正是 `^net_interface|^no_skip`，所以这个上游 `checkpoint 5` starter 对应本地 check4 的 NetworkInterface 框架。
- 本地处理：只导入缺失的 `src/network_interface.{hh,cc}`、`tests/net_interface.cc`、`tests/network_interface_test_harness.hh`，并在 `tests/CMakeLists.txt` 加 `add_test_exec(net_interface)`。没有改 check0-check3 业务实现，也没有导入 public HEAD 或 solved 实现。
- 兼容性说明：本地 util 使用 `Buffer` payload 体系，而来源测试辅助使用 `Ref<string>`/`clone`/`summary` 风格；为避免改全局 util，只在 `tests/network_interface_test_harness.hh` 和 `tests/net_interface.cc` 内做了最小测试兼容编辑。`src/network_interface.cc` 仍是 unimplemented starter 桩，没有 ARP cache、pending queue、frame 处理或 tick 过期逻辑。
- 验证：`make check3` 通过；`make check4` 编译并运行 `net_interface`，因 `DEBUG: unimplemented send_datagram called` 后没有发出期望 ARP frame 而失败，这是正确的 starter 状态。
- 经验：无法把“官方版本”作为当前可验证的绝对来源；更稳妥的证据链是“近官方镜像 + Keith Winstein 历史提交 + 模块/测试注册/本地 check 目标三方一致”。后续不要只按提交名数字机械匹配，要同时核对文件新增点、测试注册和本地 `etc/tests.cmake` 的 checkpoint 映射。

check5 / capstone 恢复记录（2026-06-20）：

- Router 来源仓库：`Richard-Qin-X/minnow`
- Router 来源提交：`f88b0f4aa8abb460d7854cc3986e14cdee654510`
- 提交信息：`CS144 Lab checkpoint 6`
- 作者：Keith Winstein
- 本地映射：上游 `checkpoint 6` 新增 `src/router.{hh,cc}` 和 `tests/router.cc`；本地 `etc/tests.cmake` 的 `check5` 正则是 `^net_interface|^router|^no_skip`，所以它对应本地 check5 Router starter。
- 本地处理：导入 `src/router.{hh,cc}`、`tests/router.cc`、`writeups/check6.md`，并在 `tests/CMakeLists.txt` 注册 `router`。`src/router.cc` 仍是 `debug("unimplemented ...")` 桩；未导入路由表、最长前缀匹配、TTL、转发等答案逻辑。
- Capstone/app 来源仓库：`Richard-Qin-X/minnow`
- Capstone/app 来源提交：`726be0a5a6d1c637394b73757d7766b0e73ae7b8`
- 提交信息：`CS144 Lab checkpoint 7`
- 作者：Keith Winstein
- 本地处理：按原始目录导入 `apps/tcp_eth_udp.cc`、`apps/fun_router.cc`，并恢复后续 app/support 所需的 starter 支撑文件：`apps/tcp_ipv4.cc`、`apps/ip_raw.cc`、`src/tcp_minnow_socket.cc`、`util/fd_adapter.hh`、`util/lossy_fd_adapter.hh`、`util/string_view_range.hh`、`util/tcp_minnow_socket.hh`、`util/tcp_minnow_socket_impl.hh`、`util/tcp_over_ip.{hh,cc}`、`util/tcp_peer.hh`、`util/tcp_segment.{hh,cc}`、`util/tun.{hh,cc}`、`util/tuntap_adapter.{hh,cc}`、`util/udinfo.hh`。这些是 app/support 框架，不是 check4/check5 解答。
- 必要兼容编辑：本地主线使用 `Buffer` 版 `Parser`/payload 体系，而上游后续 app/support 期待 scatter/gather 读写、TCP checksum buffer view、`FdAdapterConfig`、Unix local stream socket 等支撑接口；因此只在 `util/checksum.hh`、`util/file_descriptor.{hh,cc}`、`util/parser.hh`、`util/socket.{hh,cc}`、`util/tcp_config.hh`、`util/tcp_peer.hh`、`util/tcp_minnow_socket_impl.hh`、`apps/fun_router.cc`、`apps/tcp_eth_udp.cc` 做了兼容补丁。没有修改 check0-check3 的业务实现文件。
- 验证：`cmake --build build -t functionality_testing webget tcp_ipv4 ip_raw tcp_eth_udp fun_router` 通过；`make check3` 通过 37/37；`make check4` 正确失败于 `NetworkInterface` unimplemented；`make check5` 编译通过后先失败于前置 `net_interface`。单独运行 `build/tests/router_sanitized` 可看到 `unimplemented add_route()` / `unimplemented route()`，确认 Router 仍是 starter 状态。
- 经验：不要为了“尽量不改已有文件”把后续 starter 文件放到非原始位置；如果官方 starter 本来要求 `apps/`、`src/`、`util/`、`tests/` 的支撑层，就应放回原目录，并用最小兼容编辑让本地主线编译。边界不是“绝不修改已有 support 文件”，而是“不覆盖已完成业务实现、不混入答案、修改后必须验证旧 checks 仍通过”。

以后处理 check4/check5/check6/check7 时沿用同一原则：优先寻找官方或近官方镜像中对应 checkpoint 的精确 starter commit；先与本地仓库对比，只导入本地缺失的该 checkpoint starter/test/support 文件；避免直接使用公共仓库 HEAD，因为很多 public HEAD 已经包含学生解答或后续实验改动，不能整体覆盖本地仓库。如果只能找到 solved repo，也必须回溯到对应 checkpoint 的历史 starter commit；若无法回溯，则只允许在能清楚识别并清除实现代码的前提下手工提取框架文件。

每次导入后都要验证两件事：第一，之前已经完成的 checks 仍然通过；第二，新 checkpoint 在未实现 starter 逻辑时应当失败或暴露待实现测试，而不是意外通过。这样可以确认导入的是正确的实验框架，而不是混入了解答。
