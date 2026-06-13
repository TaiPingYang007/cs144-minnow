# Check2 — TCP Receiver 学习笔记（循序渐进版）

> 2026-06-12 重置：原笔记是初始化时生成的模板，不代表我的真实理解。
> 这一版从网络基础地基开始，一层一层往上盖。
>
> 两条规则：
> 1. 每个新名词出现前，先有场景和直觉，再有术语。
> 2. 每个阶段通过「理解检查」后才进入下一阶段。

---

## 学习计划总览

| 阶段 | 主题 | 回答的核心问题 | 状态 |
|------|------|----------------|------|
| 0 | 网络世界的真相 | 为什么网络会丢包、乱序？为什么需要 TCP？ | ✅ 06-12 通关 |
| 1 | TCP 的使命 + Minnow 全景 | TCP 承诺了什么？靠什么兑现？我写过的 check0/1 在其中是什么角色？ | ✅ 06-12 通关 |
| 2 | 三套编号系统 | seqno / absolute seqno / stream index 分别是什么？为什么需要三套？ | ✅ 06-12 通关 |
| 3 | 实现 wrap / unwrap | 32 位序号回绕怎么和 64 位互转？checkpoint 是干嘛的？ | ⬜ |
| 4 | 实现 TCPReceiver | receive() 要做哪些判断？ackno / window_size 怎么算？ | ✅ 06-13 通关 |

> **🏁 check2 全部通关：官方 check2 30/30（2026-06-13，Day 5 一天完成阶段 2-4）。**

预期节奏（参考，以实际为准）：
- Day 4（2026-06-12）：阶段 0 + 阶段 1
- Day 5：阶段 2 + 阶段 3（过 wrapping_integers 测试）
- Day 6：阶段 4（过全部 recv 测试）

---

## 阶段 0：网络世界的真相

目标：理解「网络是不可靠的」这个核心事实——它是 TCP 存在的全部理由。

学完要能用自己的话回答：

1. 数据在网络里是以什么形式传输的？
2. 为什么数据会乱序到达？（这正是我在 check1 里处理过的现象）
3. 网络还会出哪些「事故」？
4. IP 层承诺了什么、不承诺什么？


我的回答（学完填写）：

>   1、数据包
    2、网络传输的底座使IP协议，但是IP协议只是负责路由和转发，并不能保证顺序，网络传输的过程中可能出现拥塞（丢包），数据损坏，超时重传，多路径和网络状况等情况导致会乱序到达
    3、丢失、乱序、重复、损坏
    4、承诺了按需求转发，如果一切顺利的话，肯定能达到目的地，可以这么说吧
但是不承诺按序达到，传输时间，不重复发送，数据在传输路径上是否安全等

---

## 阶段 1：TCP 的使命 + Minnow 全景

目标：知道 TCP 对应用程序承诺了什么，靠哪几个手段兑现，以及 Minnow 各个 check 拼出来的全景图。

学完要能回答：

1. TCP 给应用程序的承诺是什么？
2. 兑现承诺的三个基本手段是什么？
3. check0 ByteStream / check1 Reassembler / check2 TCPReceiver 各自负责哪一段？
4. 一次 TCP 通信的生命周期：开始和结束时各发生什么？（SYN / FIN 的直觉版）
5. 接收方要向发送方回答哪两个问题？（ackno / window_size 的直觉版）

我的回答（学完填写）：

>   1、承诺可靠传输，可以确保网络传输过程中按序、不丢、不漏、不重的一根“水管”。
    2、确认，序号，重传
    3、check0 ByteStream 负责把数据写入缓冲区，共应用层读取，check1 Reassembler负责把传过来的乱序数据包进行排序，check2 TCPReceiver负责翻译序号+回执，翻译序号是把网络的32bit 序列号翻译成流序号64位的，回执就是告诉发送方哪些是确认接受的，这个确认接受的时机就是“push”
    4、syn是三次握手里面的，fin是四次挥手吧，我们还没有这样的概念，其实我对于tcp的细节掌握的不好，现在的理解是syn第一个发来的数据包的标志，fin就是最后一次发来的数据包的标志
    5、我已经接受的多少和我这边还有多大的窗口

---

## 阶段 2：三套编号系统

目标：分清三套编号，理解每一套存在的理由。

学完要能回答：

1. TCP 报文里的序号为什么只有 32 位？为什么从随机值（ISN）开始？
2. 为什么 SYN 和 FIN 各占一个序号？
3. 三套编号如何互相换算？（拿一个具体例子手算）

我的回答（学完填写）：

>   1、32位可以说是历史因素吧，这个不是我能决定的，32位最多位2^32-1，也就是4GB，这个范围在现在的网络传输中根本不够看，所以有回绕机制，当超出范围后可以重新从0开始继续编号，反复使用。所以需要 wrap/unwrap 翻译器一个是32位->64位，一个是64位->32位；为什么从随机值（ISN）开始？第一就是可以有一定的保护作用，如果没有这个随机值（ISN），那么序号就是从0开始，太容易暴露和太容易被攻击了，虽然我不知道怎么攻击，但是这样反正不安全，还有就是防止幽灵序号，比如这次的网络发送，因为网络传输太慢了，中间正好有一次重启，如果还是从0开始，那么自然就可能出现上一次，也就是重启前的数据，可能就被接受了，但是这是不需要的
    2、这个我的理解是对于这样的关键的标志，也应该保证可靠传输，所以需要序号
    3、传输“hi”，因为我写出来对应的名词，就凭感觉写了
                syn     'h'     'i'     fin
    seqno     isn+0   isn+1   isn+2   isn+3
    绝对序号     0        1       2       3
    流索引       -        0       1       -

---

## 阶段 3：实现 wrap / unwrap ✅ 06-13 通关（6/6 全绿）

目标：完成 `src/wrapping_integers.cc`，通过全部 wrapping_integers 测试。

- [x] 理解 wrap 的钟表模型
- [x] 理解 unwrap 为什么需要 checkpoint
- [x] 实现 wrap 并通过测试
- [x] 实现 unwrap 并通过测试

### wrap（简单）

`wrap(n, zero_point) = zero_point + (n mod 2³²)`。代码：`return zero_point + static_cast<uint32_t>(n);`
——把 uint64_t 强转成 uint32_t **就等于 % 2³²**（uint32 盒子只装得下低 32 位，高位整块自动砍掉）。

### unwrap（难，调了 5 轮）

三个参数对号入座：
- `raw_value_`（this 对象的值）= 要翻译的 32 位 seqno
- `zero_point.raw_value_` = ISN
- `checkpoint` = 当前进度参照（实际传 bytes_pushed 附近）

测试调用 `Wrap32(1).unwrap(Wrap32(0), 0)` 读法：点号**前**的 `Wrap32(1)` 才是被翻译的 seqno；
括号里 `Wrap32(0)` 是 ISN，`0` 是 checkpoint。别把 ISN 当成被翻译的值。

我的方法（数圈数）：`this_one = checkpoint / 2³²`，分三块互斥处理——
① `cp_low >= offset`（比同圈/下圈）② `cp_low < offset && this_one!=0`（比同圈/上圈）
③ `this_one==0`（比 offset 和 offset+2³²）。

### 踩坑记录（全是同一类病：无符号减法方向写错→下溢）

1. **lap 写错**：第二分支 diff2 用 last_one 算距离，赋值却写成 next_one → 答案差整整 2 圈。
2. **last_one 下溢**：`this_one==0` 时 `this_one-1` 在 uint64 下溢成天文数字 → 必须单独拆出 this_one==0。
3. **diff1 减法方向**：`checkpoint - candidate` 默认候选在下方；this_one==0 时候选(offset)常在上方
   → 下溢。改成"谁大减谁"的绝对距离才对。
4. **blanket return offset 太粗暴**：checkpoint 摸到圈顶时最近候选其实翻到下一圈（offset+2³²），
   不能无脑返回 offset。
5. **unwrap 纯粹挑最近，可前可后**：不假设"一定在 checkpoint 之后"——重传的旧数据 seqno
   会落在 checkpoint 稍前，必须能选到它（误以为"只能往后"是错的）。

### 🥋 学到的可复用招式：signed modular distance（有符号模距离）

圆环上求"离参照点最近的代表元"，**别数圈数**——把无符号差强转成有符号，硬件免费算出"最短跳跃(带正负)"：
```cpp
uint32_t offset = raw_value_ - zero_point.raw_value_;
uint32_t cp_low = static_cast<uint32_t>(checkpoint);
int32_t  hop    = static_cast<int32_t>(offset - cp_low);   // ★ 一行搞定方向+距离
int64_t  result = static_cast<int64_t>(checkpoint) + hop;
if (result < 0) result += (1LL << 32);
return result;
```
int32_t 范围恰好半圈，所以"差>半圈"自动解读为负（走近路），上面那 5 个坑**一次性全消失**。
同款 idiom：Linux 内核 `time_after(a,b)` = `(int32_t)(b-a) < 0`；环形缓冲区；角度/罗盘回绕。
（这次先用自己数圈数的版本过了；hop 版作为"成品招式"收进工具箱，下次遇到回绕+求最近直接调。）

---

## 阶段 4：实现 TCPReceiver ✅ 06-13 通关（官方 check2 30/30）

目标：完成 `src/tcp_receiver.cc`，通过全部 recv_* 测试。

- [x] 读懂 TCPSenderMessage / TCPReceiverMessage 两个结构体
- [x] 画出 receive() 的判断流程（骨架已成）
- [x] 想清楚需要哪些成员变量（→ std::optional<Wrap32> isn_）
- [x] 实现 receive()
- [x] 实现 send()
- [x] 通过全部 check2 测试（官方 check2 30/30）

### receive() 最终实现要点
- ① `if(message.SYN) isn_ = message.seqno;`（SYN 的 seqno 就是 ISN）
- ② `if(!isn_.has_value()) return;`（没原点没法翻译，丢弃）
- ③ `checkpoint = bytes_pushed() + 1`
- ④ `abs = message.seqno.unwrap(*isn_, checkpoint)`
- ⑤ `stream_index = abs + message.SYN - 1`（bool 当 0/1；带 SYN 时 +1−1 抵消）
- ⑥ `reassembler.insert(stream_index, payload, message.FIN, inbound_stream)`

### send() 最终实现要点 + 三个真实 bug（都靠跑测试抓出）
- 结构：先算 window；`!isn_.has_value()` → 返回 `{nullopt, window}`；否则 wrap 出 ackno。
- **bug1**：ackno 忘了 `wrap`——直接 `Wrap32{abs}` 没以 ISN 为原点。connect 2(ISN=89347598)
  期望 89347599 却得 1。修：`Wrap32::wrap(abs, *isn_)`。（wrap 是**静态**方法，类名::调）
- **bug2**：abs_ackno 漏了 FIN。connect 6(SYN+FIN) 期望 7 得 6。
  修：`bytes_pushed() + 1 + (inbound_stream.is_closed() ? 1 : 0)`。
- **bug3**：window 用 `static_cast<uint16_t>` 截断（max+1=65536 → 0）。
  修：`min(available_capacity, (uint64_t)UINT16_MAX)`（两参同类型，先 min 再装进 uint16）。
- 关键概念：FIN 的 +1 必须挂 `is_closed()`（流真关了），不能挂 `message.FIN`——
  FIN 乱序先到不算确认（确认=拼图进度，不是签收）。
- 易错：`std::min` 两参必须同类型（uint64_t vs int UINT16_MAX 编译不过）；需 `#include <algorithm>`。

### 全景串联（关键认知纠偏）

- **wrap/unwrap 只是两把"翻译螺丝刀"，不是 check2 主体。** 主体是 `receive()` 和 `send()` 两个调度员。
- `receive()`：收报文 → unwrap(32→64) → 减 SYN 得 stream index → 喂 `reassembler.insert()`
- `send()`：读 `bytes_pushed` → 算"下一个想要的 absolute" → wrap(64→32) → ackno
- **关键接缝：absolute → stream index 的转换在 receive() 里手写一行**（不是 unwrap 给的）：
  `payload 字节 stream index = absolute − 1`（−1 是 SYN 占走的 absolute 0）。
- **send() 读 ByteStream 不读 Reassembler**（签名只给 `Writer&`）；数据源是 `bytes_pushed()`。

### 两个消息结构体（再确认）

- `TCPSenderMessage`（输入）**只有 4 个字段**：`seqno / SYN / payload / FIN`。
  Minnow 简化了模型，**没有 ACK/RST/PSH 等标志**。receive() 要处理的就这 4 个。
- `TCPReceiverMessage`（输出）：`ackno`(optional) + `window`。**没有 SYN**——Receiver 永不产生 SYN。

### TCP 全双工 + check2 的边界

- TCP 是全双工：A→B、B→A 两条独立流，各有 ISN/SYN/FIN/编号。
- 每台主机同时演两角：Sender（管 seqno/SYN/FIN）+ Receiver（管 ackno/window）。
- 三次握手第②个报文 = SYN(来自B的Sender) + ACK(来自B的Receiver) 合体；合并是 TCPPeer 层(后面的 check)的活。
- **check2 只造一条流的 Receiver 半边**，不碰握手编排——只做：记 ISN、喂拼图、报 ackno+window。

### 成员变量设计

- reassembler / inbound_stream 都是**参数**（别人保管），不是成员。
- checkpoint 每次用 `inbound_stream.bytes_pushed()` 现算，不用存。
- **唯一要跨调用记住的是 ISN** → `std::optional<Wrap32> isn_ {}`。
- 为什么用 optional：它=「值 + bool标志」的安全打包，天然表达「没收到SYN(空)/已收到(有值)」，
  且和 ackno 的 nullopt 语义同构。空盒默认构造、赋值即填充、取值前先 `has_value()`。

### 踩坑记录

>

---

## 学习日志

### 2026-06-12（Day 4）

- 确认 check2 代码是原始模板状态（wrap/unwrap/receive/send 都是占位符）。
- 重要校准：原笔记内容是模板生成的，不是我自己的理解；我的网络基础接近零起点。
- 决定重置笔记，从阶段 0 网络地基开始循序渐进。
- 阶段 0 通关：摸底比预期好——自己答出了分组、多路径乱序、拥塞丢包；
  并自己推出了「超时重传 → 网络慢时弄巧成拙 → 重复包」，过程中自发说出"确认消息"（ACK）。
- 阶段 1 通关：自己推出了回执两要素（ackno + window_size）；理解检查通过
  （丢弃 → 从未被确认 → 发送方等不到确认 → 超时重传）。
- 关键纠偏 1：确认是「拼图进度」，不是「签收记录」——只有拼进 ByteStream 的连续字节
  才被 ackno 覆盖；暂存在 Reassembler 等缺口的乱序数据同样未被确认。
- 关键纠偏 2：网络里跑的是数据包，字节流是 TCP 给应用的假象（第 1 题已自行改对）。
- 提前命中：ackno 的来源 = bytes_pushed()（阶段 4 还有两个小补丁）；FIN ↔ is_last_substring。
- 待办：把阶段 1 的 5 个问题用自己的话填进「我的回答」。
- 明日（Day 5）：阶段 2 开场谜题——ISN 为什么随机（方向已猜对：保护 + 防串台）→ 阶段 3 动手。
- 晚间加餐：提前开进阶段 2（ISN 之谜 + 32 位回绕 + 三套编号）。
- 阶段 1 答案复查：Q1 重写合格（水管四要素），Q4 标签版合格；Q3 一处笔误仍待修：回执是告诉「发送方」。
- 阶段 2 通关：手算表 11/12，唯一错格是给 FIN 发了 stream index——
  用「两本账本」掰直：seqno 数"要运的东西"，stream index 数"能读的字节"；
  SYN/FIN 要运（必须能被确认+重传）但不可读（FIN 到 Reassembler 门口化为 is_last_substring 这个 bool）。
- 理解检查通过："重要标志也要可靠传输，可靠传输就需要编号+重传"。
- 待办：填阶段 2 的 3 个问题；阶段 1 Q3 笔误（→ 发送方）记得改。
- 睡前思考题（可选，明天阶段 3 开场用）：一条长连接里收到 seqno = 1500，
  它可能是 absolute 500、500 + 2^32、500 + 2·2^32……接收方手里有什么线索能挑出对的那个？
- 阶段 2 答案复查：Q3 表格 12/12 全对且自行抽象成 isn+偏移 形式；Q2 通过；
  Q1 待补最关键半句——32 位的后果是回绕（4 GiB 一圈），这是 wrap/unwrap 存在的原因。
- 术语纠正：表格行名「网络字节序」要改成「seqno（序号）」——
  字节序（byte order）是大端/小端那个概念，完全是另一回事；「绝对index」改「绝对序号」。
- 攻击画面补充：off-path 攻击者只能盲发伪造报文，序号可预测=盲发也能命中窗口；
  随机 ISN = 32 位暗号。

### 2026-06-13（Day 5）

- 阶段 3 通关（6/6 全绿）：wrap 一行拿下；unwrap 自己用"数圈数"方法调了 5 轮，
  全是无符号减法方向写错→下溢的同一类病（详见阶段 3 踩坑）。坚持用自己的方法压绿。
- 收获可复用招式 signed modular distance（int32_t hop），已收进工具箱，下次回绕求最近直接调。
- 阶段 4 概念铺开：纠正"wrap/unwrap 是主体"的误解（它们只是 receive/send 内部的翻译工具）；
  讲清 absolute→stream index 的接缝在 receive() 手写、send() 读 bytes_pushed 不读 Reassembler；
  讲清 TCP 全双工 + check2 只造一条流 Receiver 半边、不碰握手编排；
  TCPSenderMessage 只有 4 字段（无 ACK/RST）。
- 学了 std::optional<Wrap32>（之前没用过）：空盒/有值，赋值即填充，取值前 has_value()。
- 节奏校准：用户主动要求放慢，把零件串成全景后再写代码。剩 3-4 小时，目标当天通关 check2。
- 下一步：定成员变量 isn_ → 写 receive() 第①②步（记 ISN / 没 ISN 就 return）。

---

## 附录：概念速查（阶段 2 之后再看，现在跳过）

> 这里是原笔记浓缩下来的参考资料，等学到对应阶段再回来看。

### 三套编号对照表

| 名字 | 类型 | 谁使用 | 从哪里开始 | 是否包含 SYN/FIN |
|------|------|--------|------------|-------------------|
| 32-bit wrapping seqno | `Wrap32` | 网络上的 TCP 报文 | ISN（随机初始值） | 包含 |
| absolute sequence number | `uint64_t` | Receiver 内部中间坐标 | 0 | 包含 |
| stream index | `uint64_t` | Reassembler / ByteStream | 0 | 不包含，只数 payload 字节 |

### wrap 的钟表模型

`uint32_t` 像一个有 2^32 个刻度的钟。`wrap(n, zero_point)` = 从 `zero_point` 刻度出发顺时针走 `n` 步，落在哪个刻度就是结果。即 `(zero_point + n) mod 2^32`，C++ 无符号加法天然回绕。

例：ISN = 2^32 - 1，n = 5 → 依次走到 0,1,2,3,4 → 结果 4。

### unwrap 与 checkpoint

同一个 32-bit 值对应无穷多个 absolute seqno（x, x+2^32, x+2·2^32, ...）。checkpoint 是「当前进度附近」的参考点，unwrap 返回所有候选里最接近 checkpoint 的那个。

### 两个消息结构体（接口协议）

`TCPSenderMessage`（发送方 → 接收方）：`seqno`(Wrap32) + `SYN`(bool) + `payload`(Buffer) + `FIN`(bool)；`sequence_length() = SYN + payload.size() + FIN`。
注意：SYN 置位时 seqno 是 SYN 的编号（即 ISN）；否则是 payload 第一个字节的编号。

`TCPReceiverMessage`（接收方 → 发送方）：`ackno`(optional<Wrap32>，没收到 SYN 前为空) + `window_size`(uint16_t，最大 65535)。

### 易混淆边界（实现阶段再核对）

1. SYN/FIN 占 seqno，但不占 stream index，不进 Reassembler 的 payload。
2. 没收到 SYN 前，没有 zero_point，不能 unwrap，ackno 为空。
3. ackno 是「下一个想要的」，不是「最后收到的」；它反映拼图进度（基于 bytes_pushed），
   暂存在 Reassembler 等缺口的乱序数据不被确认。
4. FIN 只有在整条流真正组装完、Writer 关闭后才计入 ackno。
5. window_size = min(available_capacity, 65535)。
