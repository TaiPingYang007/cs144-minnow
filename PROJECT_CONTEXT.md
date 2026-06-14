# PROJECT_CONTEXT - cs144

## 项目定位

- 正式仓库：`/home/taipingyang007/projects/cpp_project/05_Stanford-CS144-Computer_Network_Systems/cs144-minnow`
- GitHub 正式仓库：`TaiPingYang007/cs144-minnow`
- 定位：使用单个正式仓库持续推进 CS144 labs，不再依赖多仓库或 starter 分支路线

## 当前进度

- check0 ByteStream：已完成
- check1 Reassembler：已完成，官方测试 18/18
- check2 TCPReceiver：已完成，官方测试 30/30
- 下一站：check3 TCPSender

当前主恢复入口已经从 check1/check2 切到 **check3: TCPSender**。

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
