# Check2 — TCP Receiver 复习笔记

> 目标：这份笔记不是学习日志，而是以后复习 check2 时可以从上到下顺读的成品。
>
> 复习顺序：先看「全貌」→ 再看「反复卡点」→ 按阶段做自测 → 最后对照代码。

---

## 0. 这关已经完成了什么

- 完成时间：2026-06-13
- 结果：官方 check2 全部通过，30/30
- 涉及代码：
  - `src/wrapping_integers.hh`
  - `src/wrapping_integers.cc`
  - `src/tcp_receiver.hh`
  - `src/tcp_receiver.cc`
- 本关核心：
  1. 把网络报文里的 32 位 TCP 序号翻译成内部可用的 64 位序号。
  2. 把乱序 payload 交给 check1 的 Reassembler。
  3. 给发送方回复：我已经连续收到哪里了，以及我还能收多少。

一句话：**TCPReceiver 是接收端的“大门 + 翻译官 + 回执员”。**

复习时不要把它背成流水线，先抓住 5 个核心判断：

1. **我有没有原点？** 没收到 SYN 就没有 ISN，不能解释任何 seqno。
2. **这个 seqno 指向谁？** 它指向包里第一个“占号元素”，可能是 SYN、payload 首字节或 FIN。
3. **我要翻译到哪套坐标？** 网络用 seqno，Receiver 中转用 absolute，Reassembler 用 stream index。
4. **我能确认到哪里？** ackno 只看连续拼好的进度，也就是 `bytes_pushed()` 和 `is_closed()`。
5. **我要怎么说给对方听？** 发回去的 ackno 必须 `wrap(abs_ackno, ISN)`，不能直接说内部 absolute。

如果这 5 个判断能顺下来，`receive()` 和 `send()` 的代码只是把它们落到实现里。

---

## 1. 复习总图：从报文到回执

### 1.1 receive() 做什么

`receive()` 处理发送方来的 `TCPSenderMessage`。

一条报文进来后，Receiver 要做四件事：

1. 如果这是 SYN，记住连接的起点 ISN。
2. 如果还没见过 SYN，非 SYN 报文没法解释，直接丢弃。
3. 把报文里的 32 位 `seqno` 翻译成 64 位 absolute seqno。
4. 再把 absolute seqno 转成 Reassembler 需要的 stream index，把 payload 塞进去。

代码位置：`src/tcp_receiver.cc` 的 `TCPReceiver::receive()`。

核心流程：

```cpp
if (message.SYN)
  isn_ = message.seqno;

if (!isn_.has_value())
  return;

uint64_t checkpoint = inbound_stream.bytes_pushed() + 1;
uint64_t absolute_index = message.seqno.unwrap(*isn_, checkpoint);
uint64_t stream_index = absolute_index + message.SYN - 1;

reassembler.insert(stream_index, message.payload, message.FIN, inbound_stream);
```

### 1.2 send() 做什么

`send()` 生成接收方要发回去的 `TCPReceiverMessage`。

它回答发送方两个问题：

1. `ackno`：我下一个想要的序号是多少？
2. `window_size`：我现在还能接收多少字节？

代码位置：`src/tcp_receiver.cc` 的 `TCPReceiver::send()`。

核心流程：

```cpp
window = min(inbound_stream.available_capacity(), UINT16_MAX);

if (!isn_.has_value())
  return { nullopt, window };

abs_ackno = inbound_stream.bytes_pushed() + 1 + (inbound_stream.is_closed() ? 1 : 0);
ackno = Wrap32::wrap(abs_ackno, *isn_);
return { ackno, window };
```

---

## 2. 三套编号：这关最重要的地基

TCPReceiver 之所以容易绕，是因为同一个“位置”在不同系统里有不同名字。

| 编号系统 | 类型 | 谁使用 | 起点 | 是否计算 SYN/FIN |
|---|---|---|---|---|
| seqno | `Wrap32` | 网络报文 | ISN | 算 SYN / FIN |
| absolute seqno | `uint64_t` | Receiver 内部中转 | 0 | 算 SYN / FIN |
| stream index | `uint64_t` | Reassembler / ByteStream | 0 | 只算 payload，不算 SYN / FIN |

### 2.1 例子：发送 `SYN 'h' 'i' FIN`

| 元素 | SYN | `'h'` | `'i'` | FIN |
|---|---:|---:|---:|---:|
| seqno | ISN + 0 | ISN + 1 | ISN + 2 | ISN + 3 |
| absolute seqno | 0 | 1 | 2 | 3 |
| stream index | - | 0 | 1 | - |

记忆句：

> **SYN / FIN 占序号，但不是数据字节。**
>
> 所以它们占 seqno 和 absolute seqno，但不占 stream index。

### 2.2 为什么需要三套编号

- 网络上的 TCP 序号只有 32 位，会回绕，所以需要 `Wrap32`。
- Receiver 内部要处理长连接，不能只有 32 位，所以需要 64 位 absolute seqno。
- Reassembler 只关心 payload 字节拼图，不关心 SYN/FIN，所以需要 stream index。

---

## 3. 反复卡点：复习时优先看这里

### ⚠️ 卡点 1：wrap / unwrap 不是“字节序”

`Wrap32` 是“会回绕的 32 位序号”，不是大端/小端那个 byte order。

- `wrap`：64 位 absolute seqno → 32 位 seqno
- `unwrap`：32 位 seqno → 64 位 absolute seqno

代码位置：`src/wrapping_integers.hh` 和 `src/wrapping_integers.cc`。

### ⚠️ 卡点 2：seqno 是“第一个占号元素”的编号

报文的 `seqno` 不是“整个包的编号”，而是这个包里**第一个占序号的东西**的编号。

- 如果 `SYN = true`，`seqno` 指向 SYN。
- 否则如果有 payload，`seqno` 指向 payload 的第一个字节。
- 如果只有 FIN，`seqno` 指向 FIN。

这和 check1 的 `first_index` 很像，只是 TCP 的 seqno 是 32 位、会回绕、还包含 SYN/FIN。

### ⚠️ 卡点 3：ISN / zero_point 是同一个“原点”

`unwrap(zero_point, checkpoint)` 里的 `zero_point` 就是 ISN。

不要把它理解成“要翻译的值”。真正要翻译的是点号前面的那个对象：

```cpp
message.seqno.unwrap(*isn_, checkpoint)
```

这里：

- `message.seqno`：要翻译的 32 位 seqno
- `*isn_`：ISN，也就是 zero_point
- `checkpoint`：当前进度附近的参考点

### ⚠️ 卡点 4：unwrap 的两个参数分工不同

`unwrap` 需要两个信息：

1. `zero_point / ISN`：决定“圈内偏移是多少”。
2. `checkpoint`：决定“应该选第几圈”。

同一个 32 位 seqno 可以对应很多 64 位 absolute seqno：

```text
x, x + 2^32, x + 2*2^32, ...
```

`checkpoint` 的作用就是从这些候选里选一个离当前进度最近的。

### ⚠️ 卡点 5：`stream_index = absolute + SYN - 1`

这是 receive() 里最容易懵的一行：

```cpp
uint64_t stream_index = absolute_index + message.SYN - 1;
```

分情况看：

- 如果报文带 SYN：
  - SYN 的 absolute seqno 是 0。
  - payload 的第一个字节才是 stream index 0。
  - 所以 `absolute + 1 - 1`，刚好把 SYN 跳过去。
- 如果报文不带 SYN：
  - payload 的 absolute seqno 比 stream index 大 1。
  - 因为 absolute seqno 里前面多算了 SYN。
  - 所以 `absolute + 0 - 1`。

本质：**absolute seqno 数 SYN，stream index 不数 SYN。**

### ⚠️ 卡点 6：FIN 的 ack 不能看当前 message.FIN

FIN 和 SYN 一样占一个序号，但 FIN 只有在整条流真的被拼完后，ackno 才能越过它。

正确依据：

```cpp
inbound_stream.is_closed()
```

不是：

```cpp
message.FIN
```

原因：FIN 可能乱序先到。如果前面还有缺口，Reassembler 还不能关闭 ByteStream，Receiver 就不能提前确认 FIN。

记忆句：

> **ackno 报的是“连续拼好的事实”，不是“我看见过某个标志”。**

### ⚠️ 卡点 7：ackno 必须 wrap 回 ISN 坐标系

内部算出来的是 absolute ackno，比如 SYN 后下一个想要的是 absolute 1。

但发回网络时，必须变回对方看得懂的 32 位 seqno：

```cpp
Wrap32::wrap(abs_ackno, *isn_)
```

如果 ISN = 89347598，SYN 后的 ackno 应该是 89347599，不是 1。

只有 ISN 恰好是 0 时，absolute 1 和网络 seqno 1 才碰巧一样。

### ⚠️ 卡点 8：window_size 要 min，不要截断

`TCPReceiverMessage.window_size` 是 `uint16_t`，最大只能表达 65535。

如果 `available_capacity()` 是 65536：

- 直接 `static_cast<uint16_t>(65536)` 会变成 0。
- 这会骗发送方：我窗口满了。
- 正确做法是先取 `min(capacity, 65535)`，再转成 `uint16_t`。

代码：

```cpp
static_cast<uint16_t>(
  std::min(inbound_stream.available_capacity(), static_cast<uint64_t>(UINT16_MAX))
)
```

---

## 4. 阶段复习

### 阶段 0：为什么需要 TCP

网络底层给的是数据包，不是可靠字节流。

IP 层大概只负责“尽力把包转发到目的地”，但不保证：

- 不丢包
- 不乱序
- 不重复
- 不损坏
- 按固定时间到达

所以 TCP 要在不可靠的网络上，给应用层制造一根“可靠、有序、不重不漏的字节流”。

#### 自测题

1. 为什么网络包会乱序？至少说出两个原因。
2. TCP 给应用层的幻觉是什么？
3. check1 的 Reassembler 是为了解决网络的哪一种现象？
4. 如果一个包丢了，发送方为什么会重传？

---

### 阶段 1：Minnow 里 check0 / check1 / check2 的关系

| Check | 组件 | 职责 |
|---|---|---|
| check0 | ByteStream | 一根有限容量的字节水管 |
| check1 | Reassembler | 把乱序 substring 拼成连续字节流 |
| check2 | TCPReceiver | 翻译 TCP 序号，把 payload 交给 Reassembler，并生成 ack/window |

关键连接：

```text
TCPSenderMessage
  -> TCPReceiver::receive()
  -> Reassembler::insert()
  -> Writer(ByteStream)

Writer(ByteStream)
  -> TCPReceiver::send()
  -> TCPReceiverMessage(ackno, window_size)
```

代码位置：

- check0：`src/byte_stream.cc`
- check1：`src/reassembler.cc`
- check2：`src/tcp_receiver.cc`

#### 自测题

1. TCPReceiver 为什么不直接把 payload 写进 ByteStream？
2. ackno 为什么来自 `bytes_pushed()`，而不是来自“收到过的最大 seqno”？
3. window_size 为什么来自 ByteStream 的剩余容量？
4. `TCPSenderMessage` 里有哪些字段？check2 为什么没有处理 ACK/RST？

---

### 阶段 2：三套编号系统

这一阶段要练到能手算。

#### 必会例题

假设 ISN = 1000，发送：`SYN 'a' 'b' 'c' FIN`。

| 元素 | SYN | `'a'` | `'b'` | `'c'` | FIN |
|---|---:|---:|---:|---:|---:|
| seqno | 1000 | 1001 | 1002 | 1003 | 1004 |
| absolute seqno | 0 | 1 | 2 | 3 | 4 |
| stream index | - | 0 | 1 | 2 | - |

#### 自测题

1. 为什么 SYN 要占一个序号？
2. 为什么 FIN 要占一个序号？
3. 为什么 SYN/FIN 不占 stream index？
4. 如果一个报文不带 SYN，payload 第一个字节的 absolute seqno 是 8，那么 stream index 是多少？
5. 如果一个报文带 SYN，payload 第一个字节的 absolute seqno 是 1，那么 stream index 是多少？

答案提示：第 4 题是 7，第 5 题是 0。

---

### 阶段 3：wrap / unwrap

代码位置：`src/wrapping_integers.cc`。

#### wrap

`wrap` 比较直接：

```text
wrap(n, zero_point) = zero_point + (n mod 2^32)
```

C++ 里把 `uint64_t` 强转成 `uint32_t`，就会保留低 32 位，相当于 `% 2^32`。

#### unwrap

`unwrap` 的核心不是“反过来算一次”这么简单，因为 32 位序号会回绕。

同一个 32 位值可能对应多个 64 位 absolute seqno，`checkpoint` 帮我们选离当前进度最近的那个。

#### 这次踩过的坑

1. 无符号整数减法方向错，会下溢成巨大数字。
2. `this_one == 0` 时再算 `this_one - 1` 会下溢。
3. 不能假设答案一定在 checkpoint 后面，重传旧包可能在 checkpoint 前面。
4. 不能粗暴返回 offset；接近圈边界时，最近值可能在下一圈。

#### 可复用招式：signed modular distance

以后遇到“环上找最近距离”，可以想这个模型：

```cpp
uint32_t offset = raw_value_ - zero_point.raw_value_;
uint32_t cp_low = static_cast<uint32_t>(checkpoint);
int32_t hop = static_cast<int32_t>(offset - cp_low);
int64_t result = static_cast<int64_t>(checkpoint) + hop;
if (result < 0) result += (1LL << 32);
return result;
```

这次最后用自己的数圈方法过了测试，但这个写法是更干净的通用工具。

#### 自测题

1. `wrap(1, ISN=1000)` 等于多少？
2. `wrap(2^32 + 5, ISN=0)` 等于多少？
3. 为什么 `unwrap` 不能只靠 `seqno` 和 `ISN`？
4. `checkpoint` 是精确答案吗？它真正的作用是什么？
5. 为什么重传包说明 unwrap 不能只往 checkpoint 后面找？

---

### 阶段 4：TCPReceiver

代码位置：`src/tcp_receiver.hh` 和 `src/tcp_receiver.cc`。

#### 成员变量

TCPReceiver 只需要长期记住一件事：ISN。

```cpp
std::optional<Wrap32> isn_ {};
```

原因：

- 没收到 SYN 前，没有原点，无法 unwrap。
- 收到 SYN 后，ISN 是整个连接的坐标原点。
- `reassembler` 和 `inbound_stream` 都是参数，不归 TCPReceiver 保管。
- `checkpoint` 每次根据 `bytes_pushed()` 现算，不需要存。

#### receive() 的关键判断

1. SYN 来了：记录 ISN。
2. 没 ISN：丢弃报文。
3. 有 ISN：unwrap 得到 absolute seqno。
4. absolute seqno 转 stream index。
5. payload + FIN 交给 Reassembler。

#### send() 的关键判断

1. window 一定可以算，即使还没 SYN。
2. 没 SYN：ackno 是 `nullopt`。
3. 有 SYN：ackno = 已经连续 push 的字节数 + SYN 占号 + FIN 可能占号。
4. ackno 发出去前要 wrap 回 `Wrap32`。

#### 三个真实 bug

1. ackno 忘记用 `Wrap32::wrap(abs, *isn_)`，导致 ISN 不为 0 时错误。
2. `abs_ackno` 漏掉 FIN，导致 SYN+FIN 这类测试差 1。
3. window 直接 `static_cast<uint16_t>`，容量 65536 被截断成 0。

#### 自测题

1. 为什么 `isn_` 要用 `std::optional<Wrap32>`？
2. 没收到 SYN 时，为什么 receive() 要丢弃非 SYN 报文？
3. 没收到 SYN 时，send() 为什么仍然可以返回 window？
4. `bytes_pushed() + 1` 里的 `+1` 是什么？
5. `+ (is_closed() ? 1 : 0)` 是什么？
6. 为什么 FIN 的确认要看 `is_closed()`？
7. 为什么 send() 不直接问 Reassembler 当前拼到哪了？
8. 为什么 window_size 不能直接转成 `uint16_t`？

---

## 5. 最小闭卷测试

复习时可以直接做这一组。能答出来，check2 的主线就基本稳了。

### 题 1：画表

ISN = 500，发送：`SYN 'x' 'y' FIN`。

请画出：

- seqno
- absolute seqno
- stream index

### 题 2：receive()

不看代码，写出 receive() 的 6 步。

重点解释这一行：

```cpp
stream_index = absolute_index + message.SYN - 1;
```

### 题 3：send()

假设已经收到 SYN，`bytes_pushed() = 10`，ByteStream 还没关闭。

1. absolute ackno 是多少？
2. 如果 ISN = 1000，发出去的 ackno 是多少？

再假设 ByteStream 已经关闭：

3. absolute ackno 又是多少？
4. 为什么多了 1？

### 题 4：FIN 乱序

如果 FIN 先到了，但前面还有缺口，Receiver 能不能马上把 ackno 越过 FIN？为什么？

### 题 5：window

`available_capacity() = 65536` 时，为什么 window_size 应该是 65535，而不是 0？

---

## 6. 复习时的代码定位

| 要复习什么 | 看哪里 |
|---|---|
| `Wrap32` 类型和接口 | `src/wrapping_integers.hh` |
| `wrap / unwrap` 实现 | `src/wrapping_integers.cc` |
| Receiver 成员变量 `isn_` | `src/tcp_receiver.hh` |
| 入站报文处理 | `src/tcp_receiver.cc` 的 `receive()` |
| ack/window 生成 | `src/tcp_receiver.cc` 的 `send()` |
| Reassembler 如何接收 payload/FIN | `src/reassembler.cc` 的 `insert()` |
| ByteStream 的 `bytes_pushed / available_capacity / is_closed` | `src/byte_stream.cc` |

---

## 7. 学习痕迹压缩版

### Day 4：建立地基

- 发现原笔记更像模板，不是自己的真实理解，于是从网络基础重建。
- 搞清楚：网络里跑的是包，字节流是 TCP 给应用层制造的假象。
- 搞清楚：ackno 不是“我见过哪些包”，而是“我已经连续拼好到哪里”。
- 提前建立了 check0 / check1 / check2 的连接：ByteStream、Reassembler、TCPReceiver 是一条流水线上的不同角色。

### Day 5：实现并通关

- 完成 wrap / unwrap，调试重点集中在 32 位回绕和无符号下溢。
- 完成 TCPReceiver，核心是 `receive()` 和 `send()` 两条路径。
- 修掉三个真实 bug：ackno 没 wrap、FIN 没计入 ack、window 截断。
- 官方 check2 30/30。

---

## 8. 最后记忆句

1. **TCPReceiver 是翻译官：网络 seqno ↔ 内部 absolute ↔ stream index。**
2. **SYN/FIN 占序号，不占 payload，不占 stream index。**
3. **ackno 是连续拼好的进度，不是见过的最大编号。**
4. **FIN 是否被确认，看 ByteStream 是否真的关闭。**
5. **window 要说实话：能表达到 65535，就用 min，不要截断成 0。**
