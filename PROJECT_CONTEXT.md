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

当前主恢复入口已切到 **check4: NetworkInterface**。⚠️ **check4 starter 缺失**：本地无 `src/network_interface.*`、无 `tests/net_interface*`，`make check4` 跑出 `net_interface (Not Run)`。开工前需按下方「Checkpoint starter 文件导入策略」，从官方镜像 check4 对应 checkpoint 提取 starter/test 文件并验证（已完成的 checks 仍过、check4 未实现时正确失败）。

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

以后处理 check4/check5/check6/check7 时沿用同一原则：优先寻找官方或近官方镜像中对应 checkpoint 的精确 starter commit；先与本地仓库对比，只导入本地缺失的该 checkpoint starter/test/support 文件；避免直接使用公共仓库 HEAD，因为很多 public HEAD 已经包含学生解答或后续实验改动，不能整体覆盖本地仓库。如果只能找到 solved repo，也必须回溯到对应 checkpoint 的历史 starter commit；若无法回溯，则只允许在能清楚识别并清除实现代码的前提下手工提取框架文件。

每次导入后都要验证两件事：第一，之前已经完成的 checks 仍然通过；第二，新 checkpoint 在未实现 starter 逻辑时应当失败或暴露待实现测试，而不是意外通过。这样可以确认导入的是正确的实验框架，而不是混入了解答。
