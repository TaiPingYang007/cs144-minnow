# CS144 — TCP/IP 协议栈实现（C++）

![C++](https://img.shields.io/badge/C%2B%2B-20-00599C?logo=cplusplus&logoColor=white)
![Build](https://img.shields.io/badge/build-CMake-064F8C?logo=cmake&logoColor=white)
![Tests](https://img.shields.io/badge/tests-CTest%20passing-success)
![Sanitizers](https://img.shields.io/badge/UBSan%20%2F%20ASan-clean-success)
![Course](https://img.shields.io/badge/Stanford-CS144-8C1515)

从零用现代 C++ 实现一套 TCP/IP 协议栈，技术重点在 **TCP 可靠传输（滑动窗口 / 超时重传 / 序号回绕）、协议分层封装与解封、ARP 地址解析与最长前缀匹配路由**。约 1k 行核心实现（`src/`），并在自建仿真网络（IP-in-Ethernet-in-UDP 隧道）上跑通**端到端 TCP 通信**——能逐包解读线上的 ARP、三次握手、超时重传与四次挥手。

> A from-scratch TCP/IP stack in modern C++, built while studying Stanford CS144.

📄 **实现细节、核心亮点与代码锚点** → [PORTFOLIO.md](./PORTFOLIO.md)

## 技术栈 · Tech Stack

C++20 · CMake · CTest · UBSan / ASan · 协议序列化与字节序处理 ·（基于 Stanford CS144）

---

## 归属 / Credit

本仓库为**学习项目**，长期公开，仅作学习用途，无意主张对课程材料的任何权利。
This is a learning project, kept public, with no claim over the course materials.

- **协议设计与 starter code**：Stanford CS144（Keith Winstein）— <https://cs144.github.io/>
- **学习路线框架与初始仓库**：rinevard/NetworkDIY — 原学习路线 README 完整保留于
  [README_NetworkDIY.md](./README_NetworkDIY.md) / [English](./README_NetworkDIY_en.md)
- **本人贡献 / My work**：`src/` 下的协议栈实现

Credit to **Stanford CS144**, **Berkeley CS168**, and **rinevard/NetworkDIY**.
