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

## 待续（后续追加）

- [ ] 三个口子 `send_datagram` / `recv_frame` / `tick` 的输入输出（对标 check3 三入口）
- [ ] check5 分阶段攻关计划（以 net_interface 测试为准）
- [ ] 带读 `src/network_interface.hh` 成员变量、动手写实现
