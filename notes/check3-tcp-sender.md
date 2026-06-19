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
| 0 | 全景 + 直觉，读 `.hh` 接口 + 拆 `send_connect`（只读不写） | — | ✅ |
| 1 | SYN：第一次 push 要发出 SYN；建立「在途序号」概念 | `send_connect` | ✅ |
| 2 | 发数据：从流取字节、按 `MAX_PAYLOAD_SIZE` 分段、受窗口约束（含 window=0 当 1 的坑） | `send_transmit` `send_window` | ✅ |
| 3 | 收 ack：`receive()` 消化 ackno/window，把已确认的段从在途队列删掉 | `send_ack` | ✅ |
| 4 | FIN：流结束且窗口允许时发 FIN | `send_close` | ✅ |
| 5 | 重传定时器：`tick()`、RTO、超时翻倍、收新 ack 重置、连续重传计数 | `send_retx` | ✅ |
| 6 | 综合边界 + 整理笔记 | `send_extra` | ✅ |

⭐ 难点预告：阶段 1 的「在途队列」和阶段 5 的「重传定时器」是 check3 真正的新东西；其余大量复用 check2 已有的 wrap / SYN·FIN 占号 / window 直觉。

---

## 0. 一句话定位（Phase 0）

TCPSender 是 ____（一句话概括它是什么、管哪个方向）。

✍️ 我的话（它在协议栈里是谁，和我写过的 TCPReceiver 是什么关系）：

> TCPSender 是 TCP 连接里负责「出向字节流」的发送端——把应用要发的字节流切成带序号的报文段,可靠地交付给对方,并管好重传与流控。
  和我写过的 TCPReceiver 的关系是互为发送、接收方，TCPSender是创造tcpmessage，TCPReceiver是接收tcpmessage并处理，TCPReceiver是发送出确认号和窗口大小，TCPSender负责处理和调整并使用重发机制保证可靠传输

⚠️ 自检探针：check2 里 `TCPSenderMessage` 是「进」TCPReceiver 的**输入**；check3 里同一个结构体变成了谁的**产物**？三个入口里谁负责产出它？

>  变成了tcpsender的产出，push负责产出

---

## 1. 它解决什么问题（Phase 0）

发送方可靠传输的三件事（先用自己的话）：

1. **发什么**：tapmessage
2. **怎么用对方的 window**：在途序列+发送序列的总大小不能超过window
3. **何时重传**：time_累计时间超过了规定时间，如果屡次超过可以判断为网络故障

🔍 回去看：[tcp_sender.hh](../src/tcp_sender.hh) 的公有方法。先光看名字猜职责，再回来对：

| 方法 | 我猜它干什么 | 谁/何时触发它 |
|---|---|---|
| `push(transmit)` | 负责组装和发送 | 应用程序往 ByteStream 写了新数据后和收到 ack 后(窗口可能变大,能发更多了) |
| `receive(msg)` | 接收TCPReceiver的“回复” | TCPReceiver的“回复”发来的时候触发，被动接收 |
| `tick(ms, transmit)` | 超时重发 | 消息发出，确认号等待时间超时触发 |
| `make_empty_message()` | 造一个零长度报文 | 发check3 里测试拿它的 .seqno 来查"你下一个要用的序号"。 |
| `sequence_numbers_in_flight()` | 报告"有多少序号已发未确认" | 上层/测试用它判断发送方状态——返回 0 = 所有数据都送达确认了 |
| `consecutive_retransmissions()` | 返回重发次数 | 发送方主动调用，超过一定次数判断网络错误 |

⚠️ 自检探针：这几个方法里哪些会「往外发报文」？发报文是用 `return` 交出去，还是通过参数里的 `transmit` 这个回调发？（提示：看 `push`/`tick` 的签名里那个 `TransmitFunction`）

> push，tick，通过参数里的 `transmit` 这个回调发

---

## 2. 两个新概念（先混个脸熟，细讲在阶段 1 / 5）

### 2.1 在途队列（outstanding segments）

⭐ 直觉种子：发出去但还没被对方 ack 的段，得自己留个底——万一丢了要照原样重发。

✍️ 我的话（为什么 Receiver 不需要这个、Sender 却需要）：

> 为什么 Receiver 不需要这个、Sender 却需要？因为Receiver只是负责被动接收，而Sender是主动发送

🔍 回去看：[tcp_sender.hh](../src/tcp_sender.hh) 现在只有 `input_ / isn_ / initial_RTO_ms_` 三个成员，这个「底账」要我自己加成员变量。

### 2.2 重传定时器（retransmission timer）

⭐ 直觉种子：发完不能干等，超时还没等到 ack，就重发最老那一段。

✍️ 我的话（计时器靠谁推进时间？是真实时钟，还是有人调 `tick` 喂时间给我？）：

> 计时器靠谁推进时间？是真实时钟，还是有人调 `tick` 喂时间给我？
  目前是“有人调 `tick` 喂时间给我”，就是test部分，但是我想真实的应该有一个计时器

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

> sequence_length() = SYN + payload.size() + FIN。

---

## 4. 分阶段（每阶段做完回来填）

### 阶段 1：SYN —— 目标 `send_connect`

🔍 回去看：[send_connect.cc](../tests/send_connect.cc)

关键问题：

- 连接刚建立，第一次 `push`，哪怕流里一个数据字节都没有，要不要发点什么出去？发什么？
- ✍️：要发syn，请求连接的标志位

⚠️ 自检探针：SYN 占 1 个序号。刚发完 SYN、还没被 ack 时，`sequence_numbers_in_flight()` 应该返回几？

> sequence_numbers_in_flight()，返回1，因为next_index已经增加，再发送之后就增加了，之后的超时重传由在途序列和push保证，而还没有确认确认号应该是0

### 阶段 2：发数据 / 分段 / 窗口 —— 目标 `send_transmit` `send_window`

🔍 回去看：[send_transmit.cc](../tests/send_transmit.cc)、[send_window.cc](../tests/send_window.cc)

关键问题（占位，做之前回来填）：

- 一次 `push` 里，怎么决定切几段、每段多大？（`MAX_PAYLOAD_SIZE` 和对方 window 谁说了算？）
- ⚠️ 窗口 = 0 时为什么要「当作 1」来试探？
- ✍️：MAX_PAYLOAD_SIZE，window，还有input_的数据大小共同决定，取最小值，如果超过MAX_PAYLOAD_SIZE那么超过了一次可发送最大区间，如果超过window，超过了发送窗口也不行，input_的数据大小更不用说了，根本就没有多的数据怎么超过？所以是这三个的最小值
  窗口 = 0 时为什么要「当作 1」来试探？
  这个是因为如果此时在途序列没有数据，那么不会有数据发送到接收方，如果接收方没有数据接收，永远不会返回一个确认好和窗口大小，这样就会发生死锁，所以应该有一个“1”探针，来让接收方触发返回确认号和窗口大小

### 阶段 3：收 ack —— 目标 `send_ack`

🔍 回去看：[send_ack.cc](../tests/send_ack.cc)

- `receive(msg)` 拿到 `ackno` 后，怎么决定哪些在途段可以从底账里删掉？
- ✍️：我们的设计是在途数据是数据包的末尾数据号+msg，如果确认号大于了这个末尾数据号那么就可以说明这个数据包接收完了

### 阶段 4：FIN —— 目标 `send_close`

🔍 回去看：[send_close.cc](../tests/send_close.cc)

- 流 `is_closed()` 后，FIN 什么时候发？窗口不够时能不能硬塞 FIN？
- ✍️：fin应该is_finished()之后再发，因为我们的check3这里还是有一个bytestream的逻辑在的，只不是应用层写，这个check3读，is_finished()的逻辑是写端关闭并且这个读端数据读完了，也就是应用层写完了，这个check3也没有要读的了，就关闭。
窗口不够时能不能硬塞 FIN？不能硬塞。FIN 占 1 个序号(sequence_length 含 FIN),所以受窗口约束。

### 阶段 5：重传定时器 —— 目标 `send_retx`

🔍 回去看：[send_retx.cc](../tests/send_retx.cc)

- `tick` 怎么累计时间、怎么判断超时？
- 超时重传后 RTO 怎么变（翻倍）？收到「推进了 ackno 的新 ack」后又怎么变（重置）？
- `consecutive_retransmissions()` 什么时候 +1、什么时候清零？
- ✍️：`tick` 怎么累计时间，现在只有test主动设置，怎么判断超时是和默认的超时时间作比较
      超时重传后 RTO 怎么变（翻倍）？收到「推进了 ackno 的新 ack」后又怎么变（重置）？
      翻倍是因为，超时重传很有可能是网络不通畅，所以选择增长等待时间，缓解网络压力。收到「推进了 ackno 的新 ack」后说明网络又畅通了，可以选择恢复，但是我认为如果保守选择的话可以不重置，等待之后连续几次超时时间内没有重传了再重置也可以，但是现在还没有设计到

### 阶段 6：综合边界 —— 目标 `send_extra`

🔍 回去看：[send_extra.cc](../tests/send_extra.cc)（31K，最大，综合所有边界）

- 做完前 5 段再来，这里是查漏。
- ✍️：

---

## 5. Sender 需要的成员变量（自己推，像 check1 那样）

⭐ 核心洞察（check1 学过）：**参数会消失，成员变量持久。** 凡是「这次调用结束后还要记住的东西」，都得是成员。

| 成员 | 类型 | 为什么需要它（跨调用要记住什么） |
|---|---|---|
| `next_seqno_` | uint64_t | 下一个要发的绝对序号（已发线） |
| `acked_seqno_` | uint64_t | 对方已确认到哪（已确认线） |
| `window_size_` | uint64_t | 对方还能收多少（流控上限） |
| `outstanding_` | queue<pair<uint64,msg>> | 在途账本：留底重传 + 算 in_flight |
| `syn_sent_` / `fin_sent_` | bool | SYN/FIN 发过没，防重发 |
| `time_` | uint64_t | 计时器累计 ms |
| `current_RTO_` | uint64_t | 当前超时门槛（超时翻倍 / 收 ack 复位） |
| `consecutive_retransmissions_` | uint64_t | 连续重传次数（超 8 判失败） |

（随推进随补；阶段 1 先填够 SYN 用的，阶段 5 再补计时器相关的。）

---

## 6. 易错点 / 踩坑记录区（check3 全程精华）

1. `if(!syn_sent_)` 要**包住整个发送块** + 别漏 `syn_sent_=true`；只包 `msg.SYN=true` 会每次 push 喷幽灵 SYN、序号爆涨 → 段错误。
2. `Wrap32(x)` ≠ `Wrap32::wrap(x, isn_)`——后者才带零点偏移。
3. `unwrap` 是**成员函数**不是静态：`msg.ackno->unwrap(isn_, checkpoint)`，不是 `Wrap32::unwrap(...)`。checkpoint **不是成员**，用 `next_seqno_` 现传。
4. `std::min` 三个值要**花括号**：`min({a,b,c})`；写 `min(a,b,c)` 第三个会被当比较器 → 编译错。
5. `reader().peek()` 返回 `string_view` → 给 `Buffer payload` 要先 `string(...)`；别忘 `pop(len)` 消费货架。
6. `outstanding_` 存的"末尾"要用 `next_seqno_ + msg.sequence_length()`（不是 `+len`，那样漏了 SYN/FIN 占位）。
7. `tick` 超时判断是 `>=` 不是 `>`（1000=1000 边界要触发）；且必须先判 `!outstanding_.empty()`，否则空队列 `front()` 段错。
8. FIN 的窗口判断：`window_room > msg.sequence_length()`（严格 `>`；别写 `a - b + 1 > 0`，无符号减法会下溢）。
9. **计时器重启**：发出"第一个在途段"（入队前 `outstanding_` 空）要 `time_=0`；否则上一轮空等累积的 `time_` 残留 → 误触发重传。
10. **window=0 不退避**：对方满是"流控"不是"网络故障"，重传探针但不 `*2`/`++`。探针 = push 当 1 挤出的普通在途段，`tick` 一视同仁重传它（没有独立"探针代码"）。
11. **非法 ack**（`new_ack > next_seqno_`）要 `return` 忽略；否则 `in_flight = next - acked` 无符号下溢成 `2^64-1`。
12. 看到 `18446744073709551615`（=2⁶⁴−1）≈ 无符号减法下溢的信号。

---

## 7. 实现进度（每次更新）

### 2026-06-20 · check3 全部通关 ✅（37/37）

- **check3 完成**：7 个 send 测试全绿 + check0/1/2 回归 + 性能/编译优化 = **37/37**。
- 最后两个边界（send_extra 才暴露）：① **RST** —— `make_empty_message` 流出错带 `RST=true`、`receive` 收 RST 则 `input_.set_error()`（`has_error`/`set_error` 就是 check0 的 ByteStream 接口，第一次用上）；② **len 扣 SYN 占位** —— 窗口上限 `window_room - msg.SYN`（SYN 也吃窗口序号预算，`window=3` 边界才暴露）。
- 下一步：**check5 = NetworkInterface（ARP / IP↔MAC）**，独立主题，另开对话。

### 2026-06-19 · check3 主体完成（6/7 绿）+ 两张全局图

**测试：6/7 绿**（send_connect / send_transmit / send_ack / send_window / send_close / send_retx），只剩 `send_extra`（综合查漏）。

#### 全局图 A：序号轴 = 滑动窗口的三条线

```
绝对序号轴: [已确认]│[在途·已发未确认]│[可发未发]│[还没产生]
                   ↑acked_seqno_     ↑next_seqno_  ↑acked_+window
```

- 在途 = `next_seqno_ - acked_seqno_`；铁律 `next_seqno_ ≤ acked_seqno_ + window`
- push 把 `next_seqno_` 右推；收 ack 把 `acked_seqno_` 右推；三线一起滑 = 滑动窗口

#### 全局图 B：重传定时器 = 盯「最老未确认段」的闹钟

- ① 发出**第一个**在途段（之前 outstanding 空）→ `time_=0`（上闹钟）
- ② tick：`time_+=ms`；若 `time_≥RTO 且 有在途` → 重传 front + `time_=0`；**`window>0` 才** `RTO×2`、`count++`（退避=故障惩罚）
- ③ 收到**推进的** ack → `time_=0` + RTO 复位 + count 清零（网络通了）

| push | 反应 | | tick | 反应 |
|---|---|---|---|---|
| window>0 有货 | 切段发(可多段) | | 空/没超时 | 啥不做 |
| window>0 无货 | 不发 | | 超时 window>0 | 重传front + 退避 |
| window=0 | 当1挤1字节探针 | | 超时 window=0 | 重传front 不退避 |

⭐ 探针 = window=0 时 push「当1」挤出的普通在途段；`tick` 重传 `front()` 时一视同仁，**没有独立探针代码**。

#### 三方法实现骨架
- `push`：`window=(window_size_==0)?1:window_size_`；`while(next<acked+window)`｛造段→`!syn_sent_`顺带 SYN→`len=min({1000,窗口剩余,货架})`→填 payload + `pop`→`is_finished && !fin_sent && window_room>seq_len` 顺带 FIN→**发前若 outstanding 空则 `time_=0`**→`transmit`→入队 `{next+seq_len, msg}`→`next+=seq_len`｝
- `receive`：ackno 有值→`new_ack=unwrap(isn_,next_seqno_)`；**`new_ack>next_seqno_` 则 return**（防下溢）；**`new_ack>acked_seqno_` 才**推进+复位计时器；删账本 `front<=acked`；更新 window
- `tick`：见全局图 B ②

### 2026-06-19 · 阶段 0 全景讲透 + 阶段 1 SYN 发送通了

- 阶段 0 全景已讲透：序号轴 + 三条线（acked_seqno_ / next_seqno_ / 窗口右界）、累积确认（Minnow 纯 cumulative ACK 无 SACK）、可靠 vs 流控两个刹车、TCPSender 在发送流水线的定位、wrap（内部 uint64 绝对序号 ↔ 线上 Wrap32 边界翻译）。
- 成员已加（tcp_sender.hh）：next_seqno_ / acked_seqno_ / window_size_ / outstanding_(queue) / syn_sent_。
- 阶段 1 已实现：`push()`（发 SYN，`if(!syn_sent_)` 包整块 + `syn_sent_=true`）、`sequence_numbers_in_flight()`（= next_seqno_ - acked_seqno_）、`make_empty_message()`（空壳只填 seqno）。
- 踩坑：① `if` 缩成只包 `msg.SYN=true`、漏写 `syn_sent_=true` → 每次 push 喷幽灵 SYN、序号爆涨 → **段错误**。教训：if 包整块、syn_sent_ 必设。② `Wrap32(x)` ≠ `Wrap32::wrap(x, isn_)`，后者才用零点偏移。③ `msg.SYN=true` 是赋值代码不是数据，别塞进 payload。
- 测试：`send_connect` 第一个 case（SYN）全绿；卡在第二个 case "SYN acked test"，因 `receive()` 还是桩，in_flight 没归 0（退出码 1，正常断言失败）。
- **明天起点：写 `receive()`** —— 取 ackno（optional，可能空）→ unwrap 成绝对序号（零点 isn_，checkpoint 用 next_seqno_）→ 更新 acked_seqno_ → 从 outstanding_ 删已完全确认的段 → 更新 window_size_。**悬而未决的设计点**：outstanding_ 光存 TCPSenderMessage 不够，删段时要判断「某段是否整段 ≤ acked_seqno_」，需要给队列元素补上每段的「绝对起始序号」。

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
  TCPSender负责读取应用层写入bytestream的数据并打包发送，同时接收TCPReceiver返回的确认号和窗口大小，TCPReceiver负责接收TCPSender发来的数据包，TCPReceiver同时处理之后的check1和check0的反馈，返回确认号和窗口大小给TCPSender，TCPSender可以调节发送数据包的大小，同时使用重传机制和在途序列保证可靠传输，“1”探针确保不会死锁

2. `push` / `receive` / `tick` 三个入口分别由谁、在什么时机触发？
  `push`由发送方再读取到bytestream的缓冲区的时候触发，"收到 ack 后"(窗口可能变大,能继续发)也会调 push。
  `receive`由TCPReceiver发送来的确认号和窗口大小被动触发
  `tick`现在都是test手动设置+=时间的，但是我认为之后应该是自动的，也就是被动触发

3. 报文是怎么发出去的——`return` 还是 `transmit` 回调？为什么这么设计？
  `transmit` 回调，用回调不用 return,是为了解耦:push 只管"造好报文",至于报文怎么送走(进测试的篮子?进真实网卡?),由调用 push 的人塞一个 transmit 进来决定。同一份 push 代码,测试和真实网络都能用、不改一行。return 的话 push 就得自己知道"怎么发",耦合死了。这叫控制反转。

4. 「在途序号」（in flight）包含 SYN/FIN 吗？为什么 Sender 要维护一个在途队列而 Receiver 不用？
  我认为「在途序号」（in flight）包含 SYN/FIN
  Sender 是发送方，需要确保可靠传输，可靠传输依赖的就是序号+超时重传+ACK 确认，所以要维护一个在途队列，Sender 发出去要为"送达"负责到被确认为止(留底重传);Receiver 收到就交付、不为别人的送达负责。责任方向相反。

5. 为什么需要重传定时器？超时后 RTO 怎么变、收到新 ack 后又怎么变？
  根本原因还是需要保证可靠传输，因为网络传输是以ip协议为基础，但是ip协议并不会保证“有序、稳定、一定到达”等问题，可能出现丢包、毁坏等情况，所以tcp的设计上需要接收方ack回复，只有回复的才是确认收到的，所以如果没有在设定的时间内收到ack就可以看作丢了，于是就需要重传定时器。超时后 RTO *2 因为超时原因可能是网络拥堵，可能是现在网络不稳定，我们发送方也应该少给网络一些“压力”，把重传时间延长、收到新 ack 后网络又畅通了，可以选择恢复，但是我认为如果保守选择的话可以不重置，等待之后连续几次超时时间内没有重传了再重置也可以，但是现在还没有设计到
