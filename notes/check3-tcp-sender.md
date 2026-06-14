# Check3 — TCP Sender 学习笔记

> 状态：模板验收通过（编译 OK、测试框架可跑、check0/1/2 全绿、`send_connect` 正确失败于 "unimplemented push()"）。进度 0/7。
> 用自己的话填写，填的过程就是检验是否真懂。卡住标 ❓，回头问太平羊。
> 「🔍 回去看」指向具体代码位置，记混了回去确认，别靠印象。
> 「⚠️ 自检探针」是给自己挖的陷阱题，填之前先答得上来。
> ⭐ 关键直觉。✍️ 我的话（这一格是我要亲手写的）。

---

## 总图：check3 七阶段路线（已和太平羊对齐）

| 阶段 | 主题 | 目标测试 | 状态 |
|---|---|---|---|
| 0 | 全景 + 直觉，读 `.hh` 接口 + 拆 `send_connect`（只读不写） | — | ⬜ |
| 1 | SYN：第一次 push 要发出 SYN；建立「在途序号」概念 | `send_connect` | ⬜ |
| 2 | 发数据：从流取字节、按 `MAX_PAYLOAD_SIZE` 分段、受窗口约束（含 window=0 当 1 的坑） | `send_transmit` `send_window` | ⬜ |
| 3 | 收 ack：`receive()` 消化 ackno/window，把已确认的段从在途队列删掉 | `send_ack` | ⬜ |
| 4 | FIN：流结束且窗口允许时发 FIN | `send_close` | ⬜ |
| 5 | 重传定时器：`tick()`、RTO、超时翻倍、收新 ack 重置、连续重传计数 | `send_retx` | ⬜ |
| 6 | 综合边界 + 整理笔记 | `send_extra` | ⬜ |

⭐ 难点预告：阶段 1 的「在途队列」和阶段 5 的「重传定时器」是 check3 真正的新东西；其余大量复用 check2 已有的 wrap / SYN·FIN 占号 / window 直觉。

---

## 0. 一句话定位（Phase 0）

TCPSender 是 ____（一句话概括它是什么、管哪个方向）。

✍️ 我的话（它在协议栈里是谁，和我写过的 TCPReceiver 是什么关系）：

>

⚠️ 自检探针：check2 里 `TCPSenderMessage` 是「进」TCPReceiver 的**输入**；check3 里同一个结构体变成了谁的**产物**？三个入口里谁负责产出它？

>

---

## 1. 它解决什么问题（Phase 0）

发送方可靠传输的三件事（先用自己的话）：

1. **发什么**：____
2. **怎么用对方的 window**：____
3. **何时重传**：____

🔍 回去看：[tcp_sender.hh](../src/tcp_sender.hh) 的公有方法。先光看名字猜职责，再回来对：

| 方法 | 我猜它干什么 | 谁/何时触发它 |
|---|---|---|
| `push(transmit)` | | |
| `receive(msg)` | | |
| `tick(ms, transmit)` | | |
| `make_empty_message()` | | |
| `sequence_numbers_in_flight()` | | |
| `consecutive_retransmissions()` | | |

⚠️ 自检探针：这几个方法里哪些会「往外发报文」？发报文是用 `return` 交出去，还是通过参数里的 `transmit` 这个回调发？（提示：看 `push`/`tick` 的签名里那个 `TransmitFunction`）

>

---

## 2. 两个新概念（先混个脸熟，细讲在阶段 1 / 5）

### 2.1 在途队列（outstanding segments）

⭐ 直觉种子：发出去但还没被对方 ack 的段，得自己留个底——万一丢了要照原样重发。

✍️ 我的话（为什么 Receiver 不需要这个、Sender 却需要）：

>

🔍 回去看：[tcp_sender.hh](../src/tcp_sender.hh) 现在只有 `input_ / isn_ / initial_RTO_ms_` 三个成员，这个「底账」要我自己加成员变量。

### 2.2 重传定时器（retransmission timer）

⭐ 直觉种子：发完不能干等，超时还没等到 ack，就重发最老那一段。

✍️ 我的话（计时器靠谁推进时间？是真实时钟，还是有人调 `tick` 喂时间给我？）：

>

---

## 3. 关键常量与消息字段（回去看，别背）

🔍 回去看：

- [tcp_sender_message.hh](../util/tcp_sender_message.hh)：我要**产出**的报文 `TCPSenderMessage`
  - 字段：`seqno`(Wrap32) `SYN` `payload`(Buffer) `FIN` `RST`
  - ⭐ `sequence_length() = SYN + payload.size() + FIN`（接 check2 的「占几个序号」）
- [tcp_receiver_message.hh](../util/tcp_receiver_message.hh)：我要**消化**的回执 `TCPReceiverMessage`
  - 字段：`ackno`(optional&lt;Wrap32&gt;) `window_size`(uint16_t) `RST`
- [tcp_config.hh](../util/tcp_config.hh)：常量
  - `MAX_PAYLOAD_SIZE = 1000`（一段 payload 的字节上限）
  - `TIMEOUT_DFLT = 1000`（默认初始 RTO，毫秒）
  - `MAX_RETX_ATTEMPTS = 8`（连续重传多少次后放弃）

⚠️ 自检探针：一个 `TCPSenderMessage` 占几个序号？SYN/FIN 算不算进去？（想 `sequence_length` 的公式）

>

---

## 4. 分阶段（每阶段做完回来填）

### 阶段 1：SYN —— 目标 `send_connect`

🔍 回去看：[send_connect.cc](../tests/send_connect.cc)

关键问题：

- 连接刚建立，第一次 `push`，哪怕流里一个数据字节都没有，要不要发点什么出去？发什么？
- ✍️：

⚠️ 自检探针：SYN 占 1 个序号。刚发完 SYN、还没被 ack 时，`sequence_numbers_in_flight()` 应该返回几？

>

### 阶段 2：发数据 / 分段 / 窗口 —— 目标 `send_transmit` `send_window`

🔍 回去看：[send_transmit.cc](../tests/send_transmit.cc)、[send_window.cc](../tests/send_window.cc)

关键问题（占位，做之前回来填）：

- 一次 `push` 里，怎么决定切几段、每段多大？（`MAX_PAYLOAD_SIZE` 和对方 window 谁说了算？）
- ⚠️ 窗口 = 0 时为什么要「当作 1」来试探？
- ✍️：

### 阶段 3：收 ack —— 目标 `send_ack`

🔍 回去看：[send_ack.cc](../tests/send_ack.cc)

- `receive(msg)` 拿到 `ackno` 后，怎么决定哪些在途段可以从底账里删掉？
- ✍️：

### 阶段 4：FIN —— 目标 `send_close`

🔍 回去看：[send_close.cc](../tests/send_close.cc)

- 流 `is_closed()` 后，FIN 什么时候发？窗口不够时能不能硬塞 FIN？
- ✍️：

### 阶段 5：重传定时器 —— 目标 `send_retx`

🔍 回去看：[send_retx.cc](../tests/send_retx.cc)

- `tick` 怎么累计时间、怎么判断超时？
- 超时重传后 RTO 怎么变（翻倍）？收到「推进了 ackno 的新 ack」后又怎么变（重置）？
- `consecutive_retransmissions()` 什么时候 +1、什么时候清零？
- ✍️：

### 阶段 6：综合边界 —— 目标 `send_extra`

🔍 回去看：[send_extra.cc](../tests/send_extra.cc)（31K，最大，综合所有边界）

- 做完前 5 段再来，这里是查漏。
- ✍️：

---

## 5. Sender 需要的成员变量（自己推，像 check1 那样）

⭐ 核心洞察（check1 学过）：**参数会消失，成员变量持久。** 凡是「这次调用结束后还要记住的东西」，都得是成员。

| 成员 | 类型 | 为什么需要它（跨调用要记住什么） |
|---|---|---|
| | | |
| | | |
| | | |

（随推进随补；阶段 1 先填够 SYN 用的，阶段 5 再补计时器相关的。）

---

## 6. 易错点 / 踩坑记录区（随时追加）

-
-

---

## 7. 实现进度（每次更新）

### 2026-06-14 · check3 启动

- 模板验收通过：编译 OK，check0/1/2 全绿（26 测试），`send_connect` 正确失败于 `unimplemented push()`。
- 已和太平羊对齐 7 阶段路线（见总图）。
- 关键事实已核对：消息字段、`sequence_length` 公式、`MAX_PAYLOAD_SIZE=1000`、`TIMEOUT_DFLT=1000`、`MAX_RETX_ATTEMPTS=8`。
- 下一步：跟着太平羊过**阶段 0 全景** → 进**阶段 1 SYN**（`send_connect`）。

测试命令：

```bash
# 跑整套 check3（含 check0/1/2 回归）
rtk cmake --build build --target check3

# 只跑单个测试（先编译该 target，再直接执行看退出码）
rtk cmake --build build --target send_connect && rtk ./build/tests/send_connect; echo "退出码: $?"
```

---

## 8. 自测验收（全填完后不看笔记口头答）

1. TCPSender 和 TCPReceiver 在同一条连接里各管哪个方向的字节流？
2. `push` / `receive` / `tick` 三个入口分别由谁、在什么时机触发？
3. 报文是怎么发出去的——`return` 还是 `transmit` 回调？为什么这么设计？
4. 「在途序号」（in flight）包含 SYN/FIN 吗？为什么 Sender 要维护一个在途队列而 Receiver 不用？
5. 为什么需要重传定时器？超时后 RTO 怎么变、收到新 ack 后又怎么变？
