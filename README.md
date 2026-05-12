NetworkDIY
==============================

[英文说明 (English Version)](./README_en.md)

这里是 CS144 和 CS168 的混合学习路线，基本思路是学 CS168 的资料而做 CS144 的作业。CS168 公开了最新年份的电子版教材和视频，授课老师幽默风趣；CS144 提供的作业基于现代 C++，因此这条学习路线能同时满足学习计网和现代 C++ 的需求。

CS144 Fall 2025 的 Starter Code 被备份在本仓库中，你可以直接 Clone 本仓库，以此为基础开始 Lab 0 的编写。实验文档及其中英对照（机翻）在 [handouts](./handouts/) 文件夹中，希望对你有帮助。

食用方法：日程表划分为了多个阶段，每个阶段的学习流程如下：
1. 学习这个阶段的 CS168 Lectures 和 Readings，可以在 [这个Youtube视频列表](https://www.youtube.com/playlist?list=PL0_XloRC3MWs5MylcqPFEeTCx0RQocrD7) 找到课程录屏。
2. 参考 [handouts](./handouts/) 里的实验文档完成这个阶段的 CS144 Lab
3. 进入下一个阶段

CS144 只有 10 周，而 CS168 有 16 周，所以在最后两个阶段会有只有课没有 Lab 的情况。祝你学得开心！

| 资源名 | 链接 |
| :--- | :--- |
| CS168 SP25 官网 | https://sp25.cs168.io/
| CS168 教材 | https://textbook.cs168.io/
| CS168 SP25 视频列表 | https://www.youtube.com/playlist?list=PL0_XloRC3MWs5MylcqPFEeTCx0RQocrD7 |
| CS144 官网 | https://cs144.github.io/ |
| CS144 Labs | https://github.com/CS144/minnow |

| 阶段 | CS144 Lab | CS168 Lectures | CS168 Readings | 备注 |
| :--- | :--- | :--- | :--- | :--- |
| P1 | Lab 0: ByteStream | 1. Intro 1: Layers of the Internet<br>2. Intro 2: Design Principles | [Intro](https://textbook.cs168.io/intro/intro.html), [Layers](https://textbook.cs168.io/intro/layers.html), [Headers](https://textbook.cs168.io/intro/headers.html)<br>[Architecture](https://textbook.cs168.io/intro/architecture.html), [Design](https://textbook.cs168.io/intro/sharing-resources.html) | Lab 0 是熟悉 C++ 环境，看导论课即可。<br><br>了解一下 C 语言的 socket 使用方法会对实现 webget\.cc 很有帮助。 |
| P2 | Lab 1: Reassembler | 3. Intro 3: Links | [Links](https://textbook.cs168.io/intro/links.html), [Multiplexing](https://textbook.cs168.io/intro/sharing-resources.html) | Lab 1 是一个纯逻辑算法题，理论需求不高，再看一节课。<br><br>在实现 ip\_raw\.cc 的过程中可能需要参考 [CS144 9月24日的 Note](./handouts/check1_addition_sep24lec.pdf)。 |
| P3 | Lab 2: TCP Receiver | 11-12. Transport 1: TCP | [Principles (Reliability)](https://textbook.cs168.io/transport/reliability.html)<br>[Design](https://textbook.cs168.io/transport/tcp-design.html), [Implementation](https://textbook.cs168.io/transport/tcp-implementation.html) | Lab 2 与 TCP 协议有关，跳跃看课。CS168 前期花了较多篇幅讲路由，而把 TCP 放在课程中期。<br><br>这个 Lab 和下一个 Lab 都不简单，可能要对着测试样例调试一阵子。 |
| P4 | Lab 3: TCP Sender | 13-15. Transport 3: Congestion Control | [Principles](https://textbook.cs168.io/transport/cc-principles.html), [Design](https://textbook.cs168.io/transport/cc-design.html), [Implementation](https://textbook.cs168.io/transport/cc-implementation.html)<br>[Throughput](https://textbook.cs168.io/transport/throughput-model.html), [Issues](https://textbook.cs168.io/transport/cc-issues.html), [Router-Assisted](https://textbook.cs168.io/transport/router-based-cc.html) | Lab 3 也与 TCP 有关，我们可以把 CS168 的 TCP 课程看完。<br><br>这是课程里最困难的一个 Lab。我们的 TCPSender 比 CS168 讲的版本要简单，但实现起来还是挺难的。祝你好运。 |
| P5 | Lab 4: Real World | 4-6. Routing 1-2: Principles, Distance-Vector | [Intro](https://textbook.cs168.io/routing/intro.html), [Model](https://textbook.cs168.io/routing/model.html), [States](https://textbook.cs168.io/routing/solutions.html)<br>[Distance-Vector](https://textbook.cs168.io/routing/distance-vector.html) | Lab 4 不需要写代码，ping 真实网站并收集数据画图。我们正好补补前期的路由课。<br><br>顺提一句，接下来的 Lab 都没那么难了。 |
| P6 | Lab 5: Network Interface | 18-20. End-to-End: Ethernet, STP, ARP, DHCP | [Ethernet](https://textbook.cs168.io/end-to-end/ethernet.html), [STP (L2 Routing)](https://textbook.cs168.io/end-to-end/l2-routing.html)<br>[ARP](https://textbook.cs168.io/end-to-end/arp.html), [DHCP](https://textbook.cs168.io/end-to-end/dhcp.html), [NAT](https://textbook.cs168.io/end-to-end/nat.html), [TLS](https://textbook.cs168.io/end-to-end/tls.html), [End-to-End](https://textbook.cs168.io/end-to-end/end-to-end.html) | Lab 5 把 IP 和 Ethernet 连接起来，我们要学学同一子网下的主机如何通信。 |
| P7 | Lab 6: IP Router | 7-8. Routing 3-4: Link-State, Addressing, Routers | [Link-State](https://textbook.cs168.io/routing/link-state.html), [Addressing](https://textbook.cs168.io/routing/addressing.html)<br>[Routers](https://textbook.cs168.io/routing/router.html) | Lab 6 要实现软件路由器，回到 CS168 前半部分看看路由课。 |
| P8 | Lab 7: Putting it together | 21-22. Datacenters: Topology, Routing | [Topology](https://textbook.cs168.io/datacenter/topology.html), [Congestion](https://textbook.cs168.io/datacenter/datacenter-cc.html)<br>[Routing](https://textbook.cs168.io/datacenter/datacenter-routing.html), [Addressing](https://textbook.cs168.io/datacenter/datacenter-addressing.html), [Virtualization](https://textbook.cs168.io/datacenter/virtualization.html) | Lab 7 不需要写代码。补下主线课。 |
| P9 | 无 Lab | 9-10. Routing (BGP)<br>16-17. Applications (DNS, HTTP)<br>23. SDN<br>24-25. Multicast | [Model](https://textbook.cs168.io/routing/autonomous-systems.html), [Design](https://textbook.cs168.io/routing/bgp.html), [Implementation](https://textbook.cs168.io/routing/bgp-implementation.html), [IP Header](https://textbook.cs168.io/routing/ip-header.html)<br>[DNS](https://textbook.cs168.io/applications/dns.html), [HTTP](https://textbook.cs168.io/applications/http.html)<br>[SDN](https://textbook.cs168.io/datacenter/sdn.html), [Host Networking](https://textbook.cs168.io/special-topics/host-networking.html)<br>[Multicast](https://textbook.cs168.io/beyond-client-server/), [Operations](https://textbook.cs168.io/beyond-client-server/collective-operations.html), [Implementation](https://textbook.cs168.io/beyond-client-server/collective-implementations.html) | Lab 做完了，补完剩余的主线课程。 |
| P10 | 无 Lab | 26. Wireless<br>27. Cellular | [Wireless](https://textbook.cs168.io/special-topics/wireless.html)<br>[Cellular](https://textbook.cs168.io/special-topics/cellular.html) | 拓展内容，Wi-Fi 和手机蜂窝移动网络，可选学习。 |

感谢 Stanford CS144 和 Berkeley CS168 开源的顶级课程资源。