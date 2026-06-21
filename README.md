# CS144 — TCP/IP 协议栈实现（C++）

基于 **Stanford CS144** 从零实现的 TCP/IP 协议栈：可靠字节流、流重组、TCP 收发（滑动窗口 / 超时重传 / 序号回绕）、IP-over-Ethernet 网络接口（ARP）、最长前缀匹配路由，并在自建仿真网络（IP-in-Ethernet-in-UDP 隧道）上跑通**端到端 TCP 通信**——能逐包解读线上的 ARP、三次握手、重传与挥手。

> This is my from-scratch implementation of a TCP/IP stack while studying Stanford CS144.

- 📄 **实现说明与技术亮点 / Implementation & highlights** → [PORTFOLIO.md](./PORTFOLIO.md)
- 📒 **逐 checkpoint 学习笔记 / Notes** → [notes/](./notes/)（重点 [check7-capstone.md](./notes/check7-capstone.md)：全栈联调 + 逐包 DEBUG 解读）

## 技术关键词

TCP/IP · 可靠传输 · 滑动窗口 · 超时重传 · 序号回绕 · ARP · 最长前缀匹配路由 · IP-in-Ethernet-in-UDP 隧道（网络仿真 / overlay）· 现代 C++

---

## 归属 / Credit

本仓库为**学习项目**，长期公开，仅作学习用途，无意主张对课程材料的任何权利。
This is a learning project, kept public, with no claim over the course materials.

- **协议设计与 starter code**：Stanford CS144（Keith Winstein）— <https://cs144.github.io/>
- **学习路线框架与初始仓库**：rinevard/NetworkDIY — 原学习路线 README 完整保留于
  [README_NetworkDIY.md](./README_NetworkDIY.md) / [English](./README_NetworkDIY_en.md)
- **本人贡献 / My work**：`src/` 下的协议栈实现 + `notes/` 学习笔记

Credit to **Stanford CS144**, **Berkeley CS168**, and **rinevard/NetworkDIY**.
