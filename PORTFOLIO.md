# CS144 — TCP/IP 协议栈实现（现代 C++ 从零实现）

> 本文件说明本仓库中**我自己完成的部分**（`src/` 协议栈实现）与**这个项目体现的技术能力**。
> 仓库基于 Stanford CS144；学习路线框架来自 rinevard/NetworkDIY（见 [README](./README.md)）。
> 诚实定位：本项目是"基于 CS144 课程的实现 + 深度原理掌握"，非原创课程或框架。

## 一句话

用现代 C++ 从零实现一套 TCP/IP 协议栈（可靠字节流 → TCP → IP/ARP → 路由），并在自建仿真网络（IP-in-Ethernet-in-UDP 隧道）上跑通**端到端 TCP 通信**——能逐包解读线上的 ARP、三次握手、超时重传、缓存过期与四次挥手。

---

## 🔧 这个项目体现的技术能力

### 现代 C++（后端核心）
- **所有权与零拷贝**：用 `move` 语义在协议各层间转移数据报 / 以太网帧的所有权，避免冗余拷贝；通过 `std::shared_ptr` 持有输出端口（OutputPort）。
- **STL 实战**：`std::unordered_map` 实现 ARP 缓存与待发队列；`std::erase_if` 安全清理过期项（规避遍历中迭代器失效）；`std::optional<Address>` 表达"可空的下一跳"。
- **健壮性**：全程 `-Werror -Wextra -Weffc++` 严格编译通过；代码在 **UBSan / ASan** 下无未定义行为（例如做子网掩码时避开 `~0u << 32` 的移位 UB）。
- 结构化绑定、`std::pair`、初始化列表、`constexpr` 常量等现代特性的实际运用。

### TCP / 可靠传输
- 实现完整 TCP 收发两端：**三次握手 / 四次挥手**、**32↔64 位序号回绕**、**滑动窗口流量控制**、**超时重传**、累积确认（ackno / window 语义）。
- 实现**乱序 / 重叠字节段的重组**（区间合并），把网络的无序交付还原成有序字节流。

### 网络协议栈与系统底层
- **IP 层**：IPv4 数据报封装、首部校验和、TTL 递减与转发。
- **链路层**：ARP 地址解析（IP↔MAC）、ARP 缓存与过期淘汰。
- **路由**：软件路由器的**最长前缀匹配（LPM）**转发——理解"按 bit 而非按字节"的掩码匹配、`/0` 默认路由。
- **封装与字节序**：协议分层封装 / 解封、序列化 ↔ 反序列化、IP-in-Ethernet-in-UDP **隧道（网络仿真 / overlay）**。

### 工程与调试
- **测试驱动**：从单元测试反推并校验实现行为（各 checkpoint 测试全绿）。
- **抓包级调试**：能逐包解读运行时流量——辨认 ARP request / reply、握手 / 挥手、缓存过期重发，并理解为何抓包里 `size=40` 的控制段分不出 SYN/FIN/ACK（标志位在 TCP 头、不占 payload）。
- **读大型代码库**：理解课程提供的适配 / 胶水层（socket、事件循环、TCP-over-IP 适配器），把自己的实现接进整套系统。

---

## 实现的模块（`src/`）

| 模块 | 文件 | 内容 |
|---|---|---|
| ByteStream | `byte_stream.*` | 有限容量的有序字节流（流量控制基础） |
| Reassembler | `reassembler.*` | 乱序 / 重叠字节段重组为连续流 |
| TCPReceiver | `tcp_receiver.*`, `wrapping_integers.*` | 32↔64 位序号回绕、ackno/window、喂 Reassembler |
| TCPSender | `tcp_sender.*` | 滑动窗口、超时重传、SYN/FIN 序号管理 |
| NetworkInterface | `network_interface.*` | IP 数据报 ⇄ 以太网帧、ARP 解析、缓存过期 |
| Router | `router.*` | 多网卡转发、最长前缀匹配（LPM） |

## 端到端验证（capstone — 对前面实现的联调，非项目主体）

> 项目主体是上面 **check0–6 的协议栈实现**；capstone 不写新算法，价值在于把它们**拼成完整系统、端到端跑通并验证**。

- **单机重演 1969 ARPANET**：UCLA(`80.6.5.4`) ↔ Stanford(`50.9.8.7`) 经一台软件路由器，双向传输数据（含 UTF-8 中文）。
- **逐包可解读**：ARP request/reply、三次握手、缓存过期重发、四次挥手——这套逐包分析我可在面试中完整复现。

## 面试可深聊的点（覆盖全栈，体现深度而非"跑通测试"）

**TCP / 可靠传输（项目核心）**
- **超时重传定时器**怎么实现？为何超时翻倍（指数退避）、收到 ACK 后如何重置与重发？
- **序号回绕**：32 位 seqno 与 64 位 absolute seqno 为何要互转？SYN/FIN 如何各占一个序号？
- **Reassembler** 如何处理乱序 / 重叠 / 重复到达的字节段（区间合并）？为什么发送方不需要它、而每个端点又都需要它（全双工 + 收 ACK 驱动重传）？

**网络 / 链路 / 路由**
- 最长前缀匹配为什么**按 bit 比、不按字节**？`~0u << 32` 为何是 UB、怎么绕？
- ARP 是 best-effort（问不到就丢包），可靠性靠什么兜底？（上层 TCP 重传，两层闭环）

**capstone 联调**
- 抓包里 `size=40` 为何分不出 SYN/FIN/ACK？两层 IP 地址（虚拟/物理）与 VPN 隧道有何相通？

> 以上每点我都能展开讲清原理、设计权衡与踩坑——欢迎面试深入追问。

## 归属

- 协议设计与 starter code 版权归 **Stanford CS144（Keith Winstein）** — <https://cs144.github.io/>
- 学习路线框架与初始仓库来自 **rinevard/NetworkDIY**（原学习路线 README 见 [README_NetworkDIY.md](./README_NetworkDIY.md)）。
- 本仓库长期公开、仅作学习用途；本人贡献为 `src/` 协议栈实现。
