# PORTFOLIO — CS144 TCP/IP 协议栈实现

> 本文件说明本仓库中**我自己完成的部分**。
> 仓库基于 **Stanford CS144** 课程；学习路线框架来自 **rinevard/NetworkDIY**（见 [README](./README.md)）。
> 我的工作 = `src/` 下的协议栈实现 + `notes/` 学习笔记。**诚实定位：本项目是"基于 CS144 课程的实现"，非原创课程或框架。**

## 一句话

基于 Stanford CS144，用现代 C++ 从零实现 TCP/IP 协议栈核心模块，并在自建的仿真网络（IP-in-Ethernet-in-UDP 隧道）上跑通**端到端 TCP 通信**——能逐包解读线上的 ARP、三次握手、重传、挥手。

## 我实现的模块（`src/`）

| 模块 | 文件 | 内容 |
|---|---|---|
| ByteStream | `byte_stream.*` | 有限容量的有序字节流（流量控制基础） |
| Reassembler | `reassembler.*` | 乱序 / 重叠字节段重组为连续流 |
| TCPReceiver | `tcp_receiver.*`, `wrapping_integers.*` | 32↔64 位序号回绕、ackno/window、喂 Reassembler |
| TCPSender | `tcp_sender.*` | 滑动窗口、超时重传、SYN/FIN 序号管理 |
| NetworkInterface | `network_interface.*` | IP 数据报 ⇄ 以太网帧、ARP 解析、缓存过期 |
| Router | `router.*` | 多网卡转发、最长前缀匹配（LPM） |

## 端到端验证（capstone）

- **单机重演 1969 ARPANET**：UCLA(`80.6.5.4`) ↔ Stanford(`50.9.8.7`) 经一台软件路由器，双向传输数据（含 UTF-8 中文）。
- **能逐包解读线上流量**：ARP request/reply、TCP 三次握手、ARP 缓存过期触发的重新解析、四次挥手——并理解为何 `size=40` 的控制段在抓包里分不出 SYN/FIN/ACK（标志位在 TCP 头、不占 payload）。
- 完整逐包解读与设计 rationale 见 [`notes/check7-capstone.md`](./notes/check7-capstone.md)。

## 技术关键词

TCP/IP · 可靠传输 · 滑动窗口 · 超时重传 · 序号回绕 · ARP · 最长前缀匹配路由 · IP-in-Ethernet-in-UDP 隧道（网络仿真 / overlay） · 现代 C++（RAII、move 语义、`std::optional` 等）

## 学习笔记（`notes/`）

每个 checkpoint 一份中文笔记，带场景比方、踩坑记录与自测题，体现对原理（而非仅"通过测试"）的理解。重点：
- [`notes/check7-capstone.md`](./notes/check7-capstone.md)：全栈联调 + 逐包 DEBUG 解读 + 一条连接的"完整一生"。
- 其余 checkpoint 笔记见 `notes/` 目录。

## 归属声明

- 协议设计与 starter code 版权归 **Stanford CS144（Keith Winstein）**。
- 学习路线框架、初始仓库结构与 `README` 来自 **rinevard/NetworkDIY**。
- 本仓库长期公开、仅作学习用途；我的贡献是上述模块的实现与学习笔记。
