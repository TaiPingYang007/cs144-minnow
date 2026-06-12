# Check1 — Reassembler 学习笔记

> 用自己的话填写。填的过程就是检验是否真懂。
> 卡住的地方标 ❓，回头问太平羊。

---

## 1. Reassembler 在 TCP 栈的位置

画出从应用层到网络层的分层图，标出 Reassembler 上下分别是谁：

``` （自下而上，我不会打那个箭头）
应用层

ByteStream

  push

Reassembler

  TCPReceiver

网络传输
```

- 它上面是：ByteStream
- 它下面是：TCPReceiver
- 它解决的核心问题（一句话）：乱序

为什么网络传来的数据会乱序？
因为网络传输的特点，数据被拆分成多个包，可以进行多路由传输，每一个包都可能使不同的路径，考虑每一个路径网络传输时间可能不同和丢包等问题，导致乱序
-

---

## 2. 两个缓冲区（今天最重要的图）

```
（你来画：ByteStream 缓冲区 vs Reassembler 缓冲区）
```

| 缓冲区 | 存什么 | 谁能读 | **代码里是什么** |
|--------|--------|--------|------------------|
| ByteStream 缓冲区 | 有序字节流 | 应用层 | `byte_stream.hh` 的 `std::string buffer_`（check0 写的）|
| Reassembler 缓冲区 | 无序/有洞的字节，按序 push 出去 | 数据被 push 进 ByteStream | `reassembler.hh` 的 `std::map<uint64_t,std::string> insert_substring_`（check1 写的）|

⭐ **抽象落地**：所有讲的"暂存区/Reassembler 缓冲区" = 那个 map `insert_substring_`。
- "暂存数据" = 往 map 里塞
- `bytes_pending()` = map 里所有 value 字符串长度之和（**只算还在 map 里的，不含已 push 进 ByteStream 的**）

关键规定：`capacity` 限制的是 ByteStream + Reassembler 的总和
（约等于真实 TCP 的什么？）读端的缓冲区
---

## 3. 三个关键数字

| 概念 | 怎么算 | 含义 |
|------|--------|------|
| next_index（左边界） | = `output.bytes_pushed()` | |
| 右边界 | = next_index + `available_capacity()` | 第一个不可存储index的编号 |
| 窗口 | `[  next_index, next_index + 可存储容量 )` | 落在里面的字节才接收|

为什么是左闭右开 `[左, 右)`？判断用哪两个不等式？
- 在窗口内：`index < 右边界`
- 在窗口外：`index >= 右边界`

### ⭐核心心法（窗口的灵魂，2026-06-11 顿悟）

insert 能不能收，看的是**「字节编号是否落在窗口 `[next_index, next_index+available_capacity())` 内」**
（**位置判断**），不是**「还剩多少名额」**（数量判断）。

为什么"看位置"自动等价于"限制总量"？
- 窗口长度 = 右边界 − next_index = `available_capacity()`
- 所有被接收的字节编号都落在这段区间 ⟹ 被接收字节数 ≤ 区间长度 = available_capacity
- 所以**不需要单独去数 Reassembler 已存了几个**，每个字节做位置判断，总量自动卡死

一个位置判断，同时解决了"乱序"和"容量"两个问题。

### capacity / available_capacity 到底是谁的？（易混！）

| 问题 | 答案 |
|------|------|
| capacity 是两缓冲区总和吗？ | **不是**。物理上只属于 ByteStream（`byte_stream.hh` 的 `capacity_`） |
| available_capacity 是两者总剩余吗？ | **不是**。只算 ByteStream 自己：= capacity − ByteStream已缓冲 |
| Reassembler 有自己的 capacity 吗？ | **没有**。容量只有一份，在 ByteStream；Reassembler 不持有容量 |
| "两者之和 ≤ capacity" 是什么？ | 是**测试对 Reassembler 的纪律要求**，靠我的代码逻辑（窗口过滤）保证 |

为什么 next_index = bytes_pushed()，而不是 +1？
因为存储是从0开始的，假设存储了0号，1号，2号，3号，一共4个包，下一个理应存储为编号为4的包

### ⭐ next_index / available_capacity 的更新时机（破除"脑袋晕"，2026-06-11）

**关键：next_index 不是手动维护的变量，它就是 `output.bytes_pushed()` 的别名。**
所以"next_index 何时变" = "bytes_pushed 何时变"，而 bytes_pushed 只有 **push 成功**才增加。

| 操作 | next_index (=bytes_pushed) | available_capacity | 原因 |
|------|---------------------------|--------------------|------|
| `push()` 进 ByteStream | **增加** ✅ | 减小 | push 是唯一让 pushed 增加的动作；同时占了 ByteStream 空间 |
| `read()`/`pop()` 应用层读走 | **不变** ❌ | **增大** | 读走的是 bytes_popped（另一本账）；腾出空间让窗口右移 |
| `insert()` 被调用 | 不一定 | 不直接变 | insert 只是入口，**内部调了 push 才变，没调就不变** |

比喻：bytes_pushed = 流水线已组装的产品总数（只增不减）；read/pop = 顾客搬走产品（不影响组装数）。

**心法：永远不要自己存 next_index 变量——随时问 `output.bytes_pushed()` 要最新值。**
**一次 insert 内部，next_index 可能涨好几次（每 push 一段就涨），所以链式 push 时要反复重读它。**

---

## 4. insert 的三种情况（设 next_index 已知）

| 情况 | 条件 | 处理 |
|------|------|------|
| A | `first_index == next_index` | 正常push呗 |
| B | `first_index < next_index`（overlap 重叠） | 截断新片段开头（砍掉 <next_index 的旧数据），剩下的部分再按 A/C 处理。**能推就立刻推，不等下次**（链式 push 已解决此疑问）|
| C | `first_index > next_index`（gap 空洞） | 等待first_index == next_index 出现|

情况C 的片段，满足什么条件才能从 Reassembler 缓冲区"转正"进 ByteStream？
-
first_index == next_index 出现
---

## 5. 待解决的细节（TODO，随学随补）

- [x] **链式 push**：每次 push 完 next_index 会变，必须**立刻**检查缓冲区有没有片段接上新 next_index，有就继续推。一次 insert 可能连推多段。
      **为什么不能等下次 insert？** 可能根本没有下次了（最后一段已到），数据会永久卡死。
      **何时停？** 两个独立原因满足任一即停：① 接不上（又遇 gap）② ByteStream 满了（available_capacity==0）。
- [x] **"满了"导致的残留会不会卡死？** 不会。因为窗口右边界用 available_capacity 算，ByteStream 满时窗口关闭(右边界=next_index)，
      "接得上的数据"要么 insert 时立即被推出（没满），要么因窗口关闭被拒收（满了）。
      → "满了 + 缓冲区有接得上的残留"这个会卡死的状态**不可能出现**。
      → Reassembler 缓冲区里留下的，永远是"接不上（有空洞）"的数据，靠链式 push + 未来 insert 解决。
      （注：②③⑤⑥ 流量控制闭环是 check2/check3 才实现，但 check1 正是依赖了这个"未来承诺"）
- [x] **部分截断**：一个片段可能被砍两刀。左砍：编号 < next_index 的旧数据。右砍：编号 ≥ 右边界的超界数据。
      中间剩下的才有效。**换算公式：`data 下标 = 全局编号 − first_index`**（把全局坐标转成 data 局部下标）。
      ⚠️ 区分三个 index：first_index(参数,全局)、next_index(=bytes_pushed,全局)、data下标(局部从0)。
      first_index 是"这次传进来的片段"的起点，**和 Reassembler 暂存区无关**！
- [x] **is_last_substring（EOF）**：close 由**我的 Reassembler 主动调 `output.close()`**（close 是 ByteStream/Writer 的方法）。
      **`.is_last()` 只是把第三个参数设成 true，是个标记/承诺，不是立即 close 的命令。**
      ⭐ 触发时机：**"流的终点字节被 push 完"才 close**，不是"收到 is_last 那次 insert"就 close。
      → 若 EOF 片段乱序到达(有空洞)，先暂存、记住标记，等空洞填上、最后字节推出后才 close。
      → **可能需要成员变量记住 is_last + 终点位置**（收到标记和真正 close 可能隔好几次 insert）。
      ⚠️ 空串 + is_last 也要 close（哪怕 BytesPushed=0）：见 single.cc "empty stream" 测试。
- [x] **数据结构选型**：用 `std::map<uint64_t, char>`（键=字节编号，值=单个字节）。
      | 操作 | map<编号,char> |
      |------|---------------|
      | 按编号有序 | ✅ map 自动升序 |
      | 查 next_index 的字节 | ✅ find / begin |
      | 插入/删除 | ✅ buffer_[i]=ch / erase |
      | **重叠自动覆盖** | ✅ key 唯一，重叠字节直接覆盖，**不用手写重叠逻辑**！|
      ⭐ 最大优点：overlap 重叠场景自动解决（TCP 保证同编号字节内容一致，覆不覆盖结果一样）。
      ❌ 代价：每字节 ~40-50 字节内存（红黑树节点开销）。第一次实现求正确，性能等过了再说。
      （另一选择 map<uint64_t,string> 内存紧凑但要手写切割，复杂，先不用。）

### Reassembler 需要的成员变量（自己推出来的）

| 成员 | 类型 | 为什么 |
|------|------|--------|
| 暂存乱序数据 | `std::map<uint64_t, char>` | 存早到但不能 push 的字节 |
| 收到 is_last 了吗 | `bool` | **is_last 是参数，用完就消失；要跨多次 insert 记住它** |
| 流的终点位置 | `uint64_t`（可选） | 判断"终点字节 push 完没"，触发 close |

⭐ 核心洞察：**参数会消失，成员变量持久。** 就像 ByteStream 用 `closed_` 记住状态，
Reassembler 也要把"会消失的 is_last 参数"抄进成员变量，才能在未来的 insert 里继续用。

---

## 6. 易错点 / off-by-one 记录

- **bytes_pending 不能用"跨度相减"算**（end编号 − begin编号），因为 buffer 有空洞，跨度≠字节数。
  必须遍历累加每段 `value.size()`。空 map 时遍历自动返回 0（不会崩），跨度算法空 map 会崩。
- **next_index 要在 while 循环【内部】每轮重读** `output.bytes_pushed()`，因为每次 push 后它会变。
- **push 后 erase 前要小心容量**：push 只写容量允许的部分，多的被丢。当前用 `available_capacity()==0` 在
  循环顶部拦截（满了就 break），避免"片段被部分push后整段erase"导致丢数据。⚠️ 但 string 版若片段比剩余
  容量大、又不是正好满 0，仍可能丢——等做窗口截取(进来时截)后彻底解决。

---

## 7. 实现进度（每次更新）

### 2026-06-11 Day 2 结束
**✅ reassembler_single 全绿（退出码0）！**

已实现（[reassembler.cc](../src/reassembler.cc)）：
- `bytes_pending()`：遍历 buffer_ 累加 ✅
- `insert` 情况A：`first_index == bytes_pushed()` → 存 buffer_ ✅
- 链式 push：while 循环 find(next_index) → push → erase ✅
- EOF：`eof_received_` + `eof_index_` 记住，终点字节 push 完调 `output.close()` ✅
- 架构选择：**统一进 buffer_ + 链式 push 统一出口**（A也进buffer_，靠链式push推出）
- 数据结构：走的是 `map<uint64_t, string>`（string 版，挑战路线）

**❌ 还没实现（明天/下次继续）：**
- [ ] **情况B**（first_index < next_index）：截掉旧数据开头
- [ ] **情况C**（first_index > next_index）：存 buffer_ 但当前没处理（现在只有 ==A 才存！）
- [ ] **窗口右边界截取**：现在的 `min(data.size(), available_capacity())` 只对情况A 正确，
      情况C 要用 `next_index + available_capacity()` 当右边界
- [ ] **重叠合并**（string 版最难）：reassembler_overlapping.cc 那个 8.3K 测试
- [ ] 跑其他测试：cap / seq / holes / dup / overlapping / win

**下次第一步**：写情况C（让 first_index > next_index 的数据也能进 buffer_），
然后跑 reassembler_seq、reassembler_holes 看效果。

测试命令：
```
rtk cmake --build build --target reassembler_single
rtk ./build/tests/reassembler_single; echo "退出码: $?"
```

### 2026-06-12 Day 3 —— ✅ check1 通关！

**单测 7/7 全绿，官方 `cmake --build build --target check1` 18/18 Passed（含 speed test）。**
Day 2 留下的 4 个 ❌（情况B/C、窗口右边界、重叠合并）全部完成。

最终架构（[reassembler.cc](../src/reassembler.cc)）：

```
A/B/C 三分支：只做两件事 → 算 start（= max(first_index, next_index)）+ 砍 data
        ↓
汇合点合并：左看一眼（std::prev 前驱）+ 右吞循环（lower_bound + erase）
        ↓
统一出口：buffer_[start] = data   （全函数唯一的存储点）
        ↓
链式 push（Day 2 写的，没动）
```

---

## 8. Day 3 核心知识（按我踩坑次数排序）

### 8.1 ⭐⭐⭐ 无符号回绕（踩了 3 次，最大的坑）

`uint64_t` 是模 2⁶⁴ 算术，**没有负数**：`10 - 12` 不是 −2，是 2⁶⁴−2（天文数字）。
钟表类比：1 点往回拨 3 小时 = 10 点。

连锁灾难链：`回绕成天文数字 → min(len, 天文数字) 选中 len → 该丢弃的数据全部保留 / substr 越界抛异常`。

| 我踩坑的现场 | 错误写法 | 后果 |
|------|----------|------|
| 情况C 窗口判断 | 先减再判 `wed_end <= 0` | `<=0` 对无符号 ≡ `==0`，只拦得住压线，拦不住回绕 |
| 情况B offset | `data.size() - offset` 没守卫 | offset > size 时回绕，substr 抛 out_of_range（dup 崩溃）|

**八字诀：先判断，再相减。** 比较两个还没减过的数永远安全：
```cpp
if ( first_index >= next_index + output.available_capacity() ) return;  // 全超窗
if ( first_index + data.size() <= next_index ) return;                  // 全旧
```

### 8.2 ⭐⭐⭐ 拼图模型 vs 数据库模型（我问得最多的地方）

我反复纠结："后到的 data 不该覆盖/更新 prev 里的旧数据吗？"——错在用**数据库思维**（新写入覆盖旧值）。

正确的是**拼图思维**：
- **序号标识位置，不是版本。** 每个编号的字节值，发送方写流那一刻就焊死了，永不改变
- 每个片段都是同一幅原图的"复印件"，重传送来的还是同样的字节，**没有"新旧"，只有"已知/未知"**
- 推论：重叠区取 prev 的字节还是 data 的字节，**结果逐字节相同**（= Day 2 写的"覆不覆盖结果一样"）
- 所以合并逻辑**只看区间几何**（起点/终点/重叠长度），从不看字符串内容
- "替换式"（prev头 + data整串）和"保留式"（prev整串 + data尾巴）等价，选下标换算少的那个

### 8.3 ⭐⭐ 左闭右开 & one-past-the-end（边界 < 还是 <= 的判据）

**`起点 + 长度` 永远是"右开边界"，不是最后一个元素。** 它本身不属于这个区间：
- 片段 `[first_index, first_index + size)`：最后一个字节的编号是 `first_index + size − 1`
- `first_index + size == next_index` 时**没有任何新字节**（全旧），所以"全旧"判断用 `<=`
- 窗口右边界 `next_index + available_capacity()` 是第一个**收不了**的编号，"全超窗"用 `>=`（镜像）

### 8.4 ⭐⭐ 区间合并设计（string 版的灵魂）

**两种死法**（裸 `buffer_[k] = data` 的下场）：
1. 同键覆盖：`{2,"cdef"}` 被 `{2,"cd"}` 覆盖 → 编号 4,5 凭空消失
2. 异键重叠：`{2,"cde"}` 和 `{4,"efg"}` 共存 → push 完 next=5，`find(5)` 找不到键 4 → 数据卡死

**不变量（一条纪律换一个循环）**：buffer_ 里任意两片段互不重叠（相邻也当场合并）。
- 该合并条件（含相邻，相邻合并无害且省条目）：`s + len >= new_start && s <= new_end`
- 推导方法：正面枚举重叠姿势必漏，**反着想"完全不挨着"只有两种摆法**，再取反

**为什么左看只需查 1 次前驱**（反证法）：前驱的前驱想够到 new_start，必须横穿前驱的身子
→ 两个条目重叠 → 不变量禁止 → 不可能存在。**想越过中间人够到远处，就必须压在中间人身上。**

**统一出口的契约**（`buffer_[start] = data` 对每个分支的三条要求）：
1. data 已是合并后的完整字节串（旧片段字节已"值拷贝"进来）
2. start 是大片段的真实起点
3. 被并入的旧条目已 erase（否则重叠复活）

流程心法：**分支算账（值拷贝进 data）→ erase 注销原件 → 统一出口一次性入库。**

### 8.5 ⭐⭐ C++ 迭代器三连

| 知识点 | 内容 |
|--------|------|
| `lower_bound(k)` | 返回**第一个键 ≥ k** 的迭代器；都 < k 则返回 end()。记法："第一个不小于 k 的位置"。孪生兄弟 `upper_bound(k)` = 第一个**严格大于** |
| lower_bound 的盲区 | 它看不见"起点在 k 左边、身子伸过来"的片段 → 左邻居要用 `std::prev(it)` 单独看 |
| `it--` vs `std::prev(it)` | 后置 `--` **先交出旧值再自减** → `auto prev = it--` 拿到的是旧位置，两变量角色互换，还可能解引用 end()（UB）。正解 `std::prev(it)`：返回前驱副本，it 不动 |
| `map::erase` | 只作废被删元素的迭代器，其他迭代器健在（节点式容器特权，vector 没有）；`it = buffer_.erase(it)` 返回下一个元素 |

### 8.6 ⭐ 工程纪律（小坑合集）

- **赋值顺序**：公式按旧 start 推导时，`start = prev->first` 必须放**最后一步**，否则公式里的 start 全被污染（我把 "b"、"d" 缝丢就是这个）
- **全局/局部下标换算**：`局部下标 = 全局编号 − 区间起点`，每写一个 substr 都默念一遍
- **变量遮蔽**：Minnow 开 `-Werror=shadow`，内层同名变量直接编译错。是好事：强迫两个生命周期不同的东西起两个名字
- **编译失败后的测试结果不可信**：跑的是旧二进制。看到 `make Error` 先停，别看退出码
- **搬家 ≠ 复制三份**：把合并逻辑剪切到三分支的**汇合点**，一份逻辑全路径生效（DRY）
- 测试通过时**一个字都不打印**：沉默 + 退出码 0 = 绿
