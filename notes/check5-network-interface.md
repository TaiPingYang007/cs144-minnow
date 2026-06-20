# check5 NetworkInterface 学习笔记

> 一句话定位：`make check5` = `handouts/check5.pdf` = CS144 **checkpoint 5** = NetworkInterface
> （ARP / IP↔MAC 地址解析 / 以太网帧封装）。
> 进度：2026-06-20 讲完全景、框架体检健康，**尚未写实现**。

---

## 0. 编号

本模块已经对齐官方编号：学习时看 `handouts/check5.pdf`，验证时跑 `make check5`。

`handouts/check4.pdf` 是官方的 "measuring the real world" 测量作业，不写
NetworkInterface 代码。

来源证据（见 PROJECT_CONTEXT.md「check5-check7 代码 starter 接入记录 2026-06-20」）：近官方镜像
`Richard-Qin-X/minnow` 历史里**没有单独的 "checkpoint 4" 提交**；NetworkInterface
是在 Keith Winstein 名为 **"CS144 Lab checkpoint 5"** 的提交
`da1014d…` 里新增 `src/network_interface.{hh,cc}` 并注册 `net_interface` 测试的。

### 🔑 要不要"重新处理"check5？——不需要

框架已经和官方 checkpoint 5 对齐。证据链三方闭合：

1. 指导书 `check5.pdf` 第 2 节描述的三个方法行为
2. ↔ `tests/net_interface.cc` 的测试用例（typical ARP workflow / pending 5s /
   active 30s …）逐条对应
3. ↔ 实跑 `make check5` 的失败信息（SendDatagram → 该广播 ARP REQUEST）完全吻合

三者一致 = 导入的是**正确的官方 NetworkInterface 实验框架**，没混答案、没缺料。
唯一要做的动作就是「看 check5.pdf，然后实现 `src/network_interface.cc`」。

---

## 1. 框架与进度验证（2026-06-20 开工前体检）

| 验证项 | 结果 | 证据 |
|---|---|---|
| 框架可编译 | ✅ | `net_interface` target 100% built；`compile with bug-checkers` Passed |
| 进度为 0 | ✅ | 运行打印 `DEBUG: unimplemented send_datagram called`，三方法全是桩 |
| 测试已注册 | ✅ | net_interface 是 ctest #35，`make check5` 能跑起来 |
| 未实现时正确失败 | ✅ | 非崩溃/编译错，而是 `ExpectationViolation: should have sent an Ethernet frame` |

结论：**干净的「待开工」状态，可以直接开始学习。**

相关文件位置：
- 要实现：`src/network_interface.cc`（桩在这）
- 接口/成员：`src/network_interface.hh`
- 测试：`tests/net_interface.cc`、`tests/network_interface_test_harness.hh`
- 数据结构（在 util/）：`arp_message.hh`、`ethernet_header.hh`、`ethernet_frame.hh`、
  `ipv4_datagram.hh`、`address.hh`

---

## 2. 全景：NetworkInterface 是什么、为什么非得有 ARP

### 它在协议栈的位置

```
check0  Application (webget)
check1-3 TCP (Sender/Receiver)          ← 已完成，产出 TCP 段
        Internet Protocol(IP)           产出 InternetDatagram(IP 数据报)
check5  ★ NetworkInterface ★            ← 现在在这：IP 数据报 ⇄ 以太网帧
        Link-layer device (NIC 网卡)
```

承上（收 IP 数据报要发）启下（封成以太网帧交网卡）。
**独立于 TCP**：测试根本不碰你的 TCP 实现；check6 Router 还会复用同一个
NetworkInterface（一个路由器有多个接口）。

### 为什么发不出去 → 为什么要 ARP（信 / 快递员比方）

你（IP 层）写好一封信（IP 数据报），信封写**最终收件人门牌号**（目的 IP）。
交给楼下快递员（网卡）时两个麻烦：
1. 快递员不认门牌号，只认**工牌号**（MAC / 以太网地址，48 位）。
2. 快递员一次只送到**下一跳** next_hop（通常是网关/路由器），不是最终目的地。

矛盾：**你知道下一跳的 IP，却不知道它的 MAC** → 以太网帧"目的地址栏"填不出 → 发不出去。

**ARP = 在本地链路广播大喊一嗓子**：
- 我吼："门牌号 192.168.0.1 是谁？工牌号报上来！"（ARP **request**，发给广播
  `ff:ff:ff:ff:ff:ff`）
- 对方回："我是，工牌 aa:ff:…"（ARP **reply**，单独回我）
- 我把这条 **IP→MAC** 记进**缓存**，下次发同一下一跳直接用、不再喊。

> 🔑 ARP 一句话：**「我有你的 IP，缺你的 MAC，在本地链路吼一声把 MAC 问出来并缓存」**。

### IP 地址 vs MAC 地址（别混，背这张表）

| | IP 地址 | MAC / 以太网地址 |
|---|---|---|
| 例子 | `4.3.2.1` | `aa:ff:a2:24:db:94` |
| 位宽 | 32 位 | 48 位（6 字节） |
| 管多远 | **端到端**，指向最终目的地 | **只管一跳**，本地链路内交接 |
| 谁用 | IP 层、路由选路 | 网卡、交换机 |
| 类比 | 门牌号（全国唯一定位） | 快递员工牌（本地交接） |
| 代码类型 | `Address` / `uint32_t` | `EthernetAddress`(=`array<uint8_t,6>`) |

直觉：**IP 是"最终去哪"，MAC 是"这一步交给谁"。** 跨一跳换一次 MAC，但 IP 头里的
源/目的 IP 全程不变。

### 测试失败 = 全景缩影（开工锚点）

`make check5` 停在第一步：
```
1. SendDatagram(dgram, next_hop=192.168.0.1)   上层要发 IP 数据报给下一跳
   期望: 发出帧 dst=ff:ff:ff:ff:ff:ff, type=ARP, opcode=REQUEST,
         target=.../192.168.0.1     ← 不知道它 MAC，所以该广播 ARP 去问
   ExpectationViolation: should have sent an Ethernet frame
```
逐字就是全景：有 IP、没 MAC、发不了帧、先广播 ARP 问。

---

## 3. 关键数据结构（代码事实，写实现时回头查）

`ARPMessage`（util/arp_message.hh，序列化长 28 字节）核心字段：
- `opcode`：`OPCODE_REQUEST=1` / `OPCODE_REPLY=2`
- `sender_ethernet_address` / `sender_ip_address`（我方）
- `target_ethernet_address` / `target_ip_address`（对方；request 时 target_eth 留空）
- `hardware_type/protocol_type/…` 有默认值，一般不用手填

`EthernetHeader`（util/ethernet_header.hh）：
- `dst` / `src`（都是 `EthernetAddress`）、`type`
- 常量：`TYPE_IPv4=0x800`、`TYPE_ARP=0x806`；`ETHERNET_BROADCAST=ff:ff:ff:ff:ff:ff`

`EthernetFrame`（util/ethernet_frame.hh）：`{ header, payload(vector<Buffer>) }`

`Address`（util/address.hh）：`ipv4_numeric()` → 32 位整数；`ip()` → 点分字符串。
帮助函数：`serialize(...)` 把 ARP/datagram 变 `vector<Buffer>` 当 payload；
`parse(obj, payload)` 反过来，返回 bool 表示成功。

---

## 自测题（第 1 轮 · 全景）

1. `make check5` 对应官方哪个 checkpoint？该看哪个 PDF？
2. 已经知道下一跳的 IP 了，为什么还发不出以太网帧？ARP 补上了哪块拼图？
3. IP 地址和 MAC 地址，各自"管多远"？为什么一个数据报跨多跳时 MAC 会变、IP 不变？
4. ARP request 的以太网目的地址该填谁？为什么是它？ARP reply 又发给谁？

<details>
<summary>参考要点（先自己答，再对照）</summary>

1. 官方 checkpoint 5；看 `handouts/check5.pdf`。官方 checkpoint 4 是 measuring
   作业，不是 NetworkInterface。
2. 缺下一跳的 MAC；以太网帧 dst 栏要 MAC。ARP 把"下一跳 IP→MAC"这块查出来并缓存。
3. IP 端到端（指最终目的地），MAC 只管一跳（本地交接）。每跳的链路双方不同，故逐跳
   换 MAC；而最终目的地不变，故 IP 头里的源/目的 IP 全程不变。
4. request 填广播 `ff:ff:ff:ff:ff:ff`（不知道对方 MAC，只能让链路上所有人都看到、由
   匹配 IP 的那个来回）；reply 单独发回给发问者的 MAC（已经从 request 里学到了）。
</details>

---

## 4. 三个口子（对标 check3 TCPSender 三入口）

NetworkInterface 不写主循环，是被外面"戳"的对象（和 TCPSender 同构）：

| check3 `TCPSender` | check5 `NetworkInterface` | 角色 |
|---|---|---|
| `push()` 上层要发数据 | `send_datagram(dgram, next_hop)` 上层要发 IP 数据报 | 出方向 |
| `receive(msg)` 收到 ack | `recv_frame(frame)` 收到以太网帧 | 入方向 |
| `tick(ms)` 触发重传 | `tick(ms)` 触发 ARP 缓存过期 | 时钟 |

逐口输入输出：
- **`send_datagram(dgram, next_hop)`**：缓存命中 next_hop 的 MAC→直接封 IPv4 帧发；
  未命中→广播 ARP request 问 next_hop 的 MAC + 数据报入队等回复。
- **`recv_frame(frame)`** 按 `header.type` 分两路：
  - `TYPE_IPv4`：只看**帧的 dst_mac 是不是我**（或广播），是就把数据报原样推进
    `datagrams_received_`。**不验数据报的 dst_ip**（那是上层 IP/路由的事；check6 路由器
    收到的帧目的 IP 故意不是自己）。
  - `TYPE_ARP`：无条件**学** sender 的 IP→MAC；若是 request 且 target_ip==我的 IP→回 reply；
    学完把队列里等这个 IP 的包发掉。
- **`tick(ms)`**：推进内部时间，清理过期的 ARP 缓存（30s）和过期的 pending 请求（5s）。

它自己拥有两块状态：① **ARP 缓存** `IP→(MAC, 学到时刻)` ② **等待队列 + 已问记录**。

### 🔑 三颗关键螺丝（第一次最容易拧错）

1. **数据报里装的是最终 dst_ip，不是 next_hop。** 例：dgram.dst_ip=13.12.11.10、
   next_hop=192.168.0.1 是两个不同 IP。ARP 问的是 **next_hop(192.168.0.1)**，因为最终
   目的地不在本地链路上、广播问它毫无意义。next_hop 不写进任何字节，只用来：(a) ARP 查 MAC，
   (b) 填进帧的 dst_mac，用完即弃。每一跳是新的 NetworkInterface 拿自己的 next_hop 重新封帧
   → 这就是"MAC 每跳变、IP 不变"。
2. **收 IPv4 帧只验 dst_mac，不验 dst_ip。** 分层职责：链路层只管这一跳交接对不对，"该不该
   收/可不可达"是 IP 层/路由(check6)的事。
3. **这层是 best-effort（故意不保证送达）。** ARP 5s 问不到就默默丢包、不报错。可靠性靠上层
   TCP 重传(check3)兜底——这就是当年逼你写重传定时器的根本原因，两个 check 在此闭环。

分层定位：传输层 TCP(check0-3) ↑ 网络层 IP(数据报) ↑ **链路层 以太网+ARP(check5 当前)**。

---

## 5. 代码类型映射（名词 → 真实类型，写实现回头查）

| 全景概念 | 代码类型 | 关键字段 / 方法 |
|---|---|---|
| MAC | `EthernetAddress` | `std::array<uint8_t,6>` |
| 广播地址 | `ETHERNET_BROADCAST` | `ff:ff:ff:ff:ff:ff` 常量 |
| 帧头 | `EthernetHeader` | `{dst,src,type}`；`TYPE_IPv4=0x800`/`TYPE_ARP=0x806` |
| 帧 | `EthernetFrame` | `{header, payload(vector<Buffer>)}` |
| IP 数据报 | `InternetDatagram` | `{header, payload}`；header 有 `src`/`dst`(uint32) |
| IP 地址 | `Address` | `.ipv4_numeric()`→u32；`.ip()`→串；`from_ipv4_numeric(u32)` |
| ARP 消息 | `ARPMessage` | `opcode`(REQUEST=1/REPLY=2)、`sender_/target_ethernet_address`、`sender_/target_ip_address`(u32)；前 4 字段有默认值别碰 |

⚠️ `ARPMessage` 里 IP 是 `uint32_t`，`next_hop` 是 `Address` → 用 `next_hop.ipv4_numeric()` 互转。

三座桥梁（字节 ⇄ 结构体，封帧/拆帧的全部动作）：
```cpp
serialize(arp_or_dgram)   → vector<Buffer>    // 结构体→字节，塞进 frame.payload
parse(obj, frame.payload) → bool              // 字节→结构体，true=成功
```
封帧：`frame.header.type=TYPE_IPv4; frame.payload=serialize(dgram);` 再填 src/dst MAC。
拆帧：先看 `header.type`，再 `parse(arp/dgram, frame.payload)`。

---

## 6. 分阶段攻关计划（按 net_interface.cc 测试难度递增）

**阶段 A — 收发直路 + ARP 基本问答（先不管时间）**
建两块状态(缓存+队列)，跑通三个口子主干：
- `send_datagram`：命中→封 IPv4 帧发；未命中→广播 ARP request + 入队
- `recv_frame` 收 ARP：学 sender 映射；request 问我→回 reply；学完发掉队列里等它的包
- `recv_frame` 收 IPv4：dst_mac 是我→推入 datagrams_received_
- 测试：typical ARP workflow / reply to ARP request / learn from ARP request /
  multiple & out-of-order pending / 独立 mapping / broadcast 学习 / unrequested reply

**阶段 B — 加时间维度（tick + 过期）**
两块状态盖时间戳：缓存活 30s 过期删；pending 5s 冷却(内不重复问、超时丢包)
- 测试：active mappings last 30s / pending last 5s / update timestamp / dropped when expires

**阶段 C — 边角 & 压力**
- 学习"不是问我的"广播 ARP；50 次循环压力测试

---

## 自测题（第 2 轮 · 三入口 + 名词 + 代码）

1. dgram.dst_ip=13.12.11.10、next_hop=192.168.0.1，ARP 该问谁的 MAC？为什么不是另一个？
2. next_hop 这个值最终会出现在发出去的帧的哪个字段里？它本身会被传到对方机器吗？
3. 收到一个 type=IPv4 的帧，要检查什么、不检查什么？为什么不验 dst_ip？
4. ARP 5 秒没人回复，排队的数据报怎么办？这种"不保证送达"靠哪一层、你哪个 check 兜底？
5. `ARPMessage` 里 IP 字段是什么类型？和 `next_hop` 的 `Address` 怎么互转？
6. 封一个 IPv4 帧，payload 怎么从 `InternetDatagram` 得到？拆帧怎么还原？

<details>
<summary>参考要点</summary>

1. 问 next_hop(192.168.0.1)；最终目的地不在本地链路，广播问它没人应。
2. 填进帧头 dst_mac；不会，对方用"自己的 MAC"收下，看不到 next_hop 概念，next_hop 用完即弃。
3. 只验帧的 dst_mac 是不是我(或广播)；不验 dst_ip。分层：收不收/可达是 IP 层/路由的事。
4. 默默丢包不报错(best-effort)；靠传输层 TCP 重传(check3)兜底。
5. `uint32_t`；`next_hop.ipv4_numeric()` 得 u32，反向 `Address::from_ipv4_numeric(u32)`。
6. `frame.payload = serialize(dgram)`；`parse(dgram, frame.payload)` 还原(返回 bool)。
</details>

---

## 7. 实现进度（2026-06-20）

- ✅ **`send_datagram` 完成并通过第 1 幕**。成员变量：`arp_table_`(`unordered_map<uint32_t, EthernetAddress>`)
  + `pending_datagrams_`(`unordered_map<uint32_t, vector<InternetDatagram>>`)。逻辑：命中→封 IPv4 帧发；
  未命中→入队 + 广播 ARP request（`already_asked` 防重复问）。
- ✅ `typical ARP workflow` 步骤 1-5 全绿，失败点后移到**步骤 6**（收 ARP reply 该发排队包）。
- ✅ **`recv_frame` 完成**：dst 判断(我/广播)→按 type 分；IPv4→push `datagrams_received_`；
  ARP→学映射(存 `sender_*`，对方)+冲刷 `pending_`(发完 `erase`)+(request 且 target==我)回 reply。
  踩坑：学映射存的是 `sender_*`(对方)不是 `ip_address_/ethernet_address_`(自己)；冲刷条件只看
  `pending_.contains`；回 reply 要 `transmit(reply_frame)` 别发原帧；shadow 命名；冲刷后 `erase`。
- ✅ **阶段 A 全通**：typical ARP workflow / reply to ARP request / learn from ARP request 三测试过。
- ✅ **阶段 B 完成**：`tick`——全局时钟 `current_time_ += ms`；`std::erase_if` 清理过期的 pending(5s)/arp_table_(30s)。
  pending 加发起时刻 `time_`(只第一次问时设，别每次刷新)；arp_table_ value 改 `std::pair<MAC, 学到时刻>`。
- 🎉 **check5 全部通关：100% tests passed**（net_interface + no_skip），2026-06-21。NetworkInterface 完成。
- C++ 踩坑沉淀：变量 shadow / 遍历中 erase 迭代器失效(改 `std::erase_if`) / `-Weffc++` 要求成员 `{}` 默认初始化 /
  `.hh` 无 `using namespace std` 要写 `std::pair`。
- 📌 架构认知：一个 NetworkInterface 对象 = 一台机器(A)的网卡；send_datagram/recv_frame 是 A 的两只手，
  共享 `arp_table_`/`pending_`。`ip_address_`/`ethernet_address_`=A自己(焊死)；`sender_*`=每帧的对方。

## 8. 类型速查（逐行精读 util 头文件的要点）

### util/ethernet_header.hh
- `using EthernetAddress = std::array<uint8_t,6>;` — MAC = 48 位 = **6 字节**；每字节 `uint8_t`(0~255)。
  显示 `42:48:58:81:1d:68` 的 6 段就是数组的 6 个元素（十六进制）。用定长 `array` 而非 `string`：精确对应网线上的字节。
- `ETHERNET_BROADCAST = {0xff ×6}` — 广播 MAC `ff:ff:..`，全 1 = 发给本地链路所有人。`constexpr` = 编译期常量。
- `to_string(EthernetAddress)` — 把 6 字节数组转成可读串 `"42:48:.."`（**声明**要懂，实现黑盒）。
- `struct EthernetHeader`（帧头 / 面单，固定 14 字节）：
  - 常量：`LENGTH=14`；`TYPE_IPv4=0x800`；`TYPE_ARP=0x806`（`uint16_t`，告诉收方载荷是哪种）
  - 字段：`dst` / `src`（`EthernetAddress`，目的 / 源 MAC）、`type`（`uint16_t`）
  - 方法：`to_string()` / `parse()` / `serialize()`（字节 ⇄ 结构体，黑盒用）
- `类型 变量名;` = 造一个该类型的空盒子起名；`盒子.字段 = 值` 往里填。

### util/ethernet_frame.hh
- `struct EthernetFrame { EthernetHeader header; vector<Buffer> payload; }` = **面单 + 货**（套娃落地）。
- ⚠️ `vector<Buffer>` 是「**一个**载荷切成的几段字节」，**不是多个数据报**（一份货的多张纸）。
  别和 `pending_datagrams_` 的 `vector<InternetDatagram>`（**多个**数据报）混。
- `Buffer` = 一段字节；`serialize(dgram/arp)` 返回的正是 `vector<Buffer>`，所以能直接赋给 `frame.payload`。
- 成员 `parse`/`serialize` = 整个帧 ⇄ 网线字节（底层收发用，你不调）；你调的是**自由函数** `serialize(x)`/`parse(obj, payload)`。

### util/arp_message.hh
- 常量：`LENGTH=28`；`OPCODE_REQUEST=1`/`OPCODE_REPLY=2`。
- 前 4 字段（`hardware_type`/`protocol_type`/两个 `*_address_size`）都有默认值（以太网+IPv4，MAC6/IP4），**不用填**。
- 核心 4 字段（要填）：`opcode`、`sender_ethernet_address`(MAC)、`sender_ip_address`(**uint32**)、
  `target_ethernet_address`(MAC)、`target_ip_address`(**uint32**)。`sender_*`=发消息的人(填自己)，`target_*`=要找的人。
- **request vs reply 字段对照**（写 recv_frame ARP 分支的命根子）：
  | 字段 | REQUEST | REPLY |
  |---|---|---|
  | opcode | REQUEST | REPLY |
  | sender_eth | 我的MAC | 对方MAC |
  | sender_ip | 我的IP | 对方IP |
  | target_eth | 空{} | 我的MAC |
  | target_ip | 要找的IP | 我的IP |
- 规律：`sender` 永远是发这条消息的人；request 的 `target_eth` 留空（正要问）；reply 把答案填进自己 `sender_eth`。
- 收到任何 ARP（request 或 reply）→ 都能从 `sender_ip → sender_eth` 学到一条映射。

---

## 自测题（第 3 轮 · 完整实现，通关后复习）

1. `recv_frame` 收到一个帧，**整体处理流程**是什么？（第一关判什么、然后按什么分叉、各分支干啥）
2. ARP 分支的**三步**是什么顺序？「学映射」为什么是**无条件**的、存的是**谁**的 IP→MAC？
3. 「冲刷 pending」为什么必须在 `recv_frame` 做、**不能**放进 `send_datagram`？（说出那个致命反例）
4. 「重问」是什么？**谁负责删、谁负责问**？如果删完之后没有新的 `send_datagram`，还会重问吗？
5. `tick` 管哪**两种过期**、各多少秒？为什么遍历容器删元素要用 `std::erase_if` 而不是 range-for + `erase`？
6. `send_datagram` 命中/未命中缓存各干什么？`already_asked` 防的是什么？`pending` 的 `time_` 为什么只在「第一次问」时设？
7. 回 ARP reply 的条件是 `target_ip==我 && opcode==REQUEST`——那个 `opcode==REQUEST` 去掉会怎样？

<details>
<summary>参考要点（先合上代码自己答，再对照）</summary>

1. ① dst 是我或广播？否→丢。② 按 `type`：IPv4→`parse` 成 dgram→push `datagrams_received_`；ARP→进 ARP 三步。
2. 顺序：先学映射→再冲刷 pending→（仅 request 且 target==我）回 reply。学映射无条件，因为链路上路过的 ARP 都白送 `sender` 的 IP→MAC，学下来省事（测试 `Learn from broadcast ARP request not for our IP` 要求）。存的是**对方**(`sender_*`)，不是自己(`ip_address_/ethernet_address_`)。
3. 因为「现在能发了」这个事件发生在「MAC 到达」的时刻（recv 到 reply），不是「新包到来」的时刻。反例：typical workflow 第 3 幕收到 reply 后**立刻** ExpectFrame，而此刻没有新的 send 调用——放 send 就发不出去。
4. 重问 = 重新广播 ARP 问同一个 IP。`tick` 负责删过期 pending；`send_datagram` 负责（删完后、新包来时）重问——靠 `already_asked` 变 false 自然走广播。**没有新包就不会重问**。
5. pending 的 ARP 请求 5 秒、arp_table_ 缓存 30 秒。range-for + `erase` 会让迭代器失效（未定义行为）；`std::erase_if` 内部安全处理。
6. 命中→封 IPv4 帧直接发；未命中→入队 + 广播 ARP。`already_asked` 防对同一 IP 重复广播。`time_` 只在第一次设，否则每来一个包就刷新计时、永远到不了 5 秒。
7. ARP reply 的 `target_ip` 也可能是我（typical 第 3 幕 B 回我的 reply）；不判 `opcode` 就会把别人的「回答」误当「提问」去回 reply，多发一个 ARP 帧→紧跟的 `ExpectNoFrame` 失败。
</details>
