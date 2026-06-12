# Check0 — ByteStream 回顾笔记

> 状态：实现完成，`ctest -R '^byte_stream_'` 9/9 全绿。
> 这份笔记由我（王吉星）用自己的话填写。空格 `____` 和「✍️ 我的话」区域是我要填的；
> 「🔍 回去看」指向 byte_stream.hh / .cc 的具体位置，记混了就回去确认，不要靠印象。
> 「⚠️ 自检探针」是给自己设的陷阱问题，填之前先答得上来。

---

## 0. 一句话定位

ByteStream 是 网络字节流（用一句话概括它是什么）。

✍️ 我的话（它在 TCP 协议栈里处于什么位置、为什么 check0 要先做它）：

> TCP 提供给应用的那根"可靠、有序的字节管道"的本体
> 为什么先做？我认为是比较基础，这个是一个最小版？

⚠️ 自检探针：它存的是「消息」还是「无边界的字节」？这个区别为什么重要？（提示：想想后面 Reassembler 拼接时还认不认得当初是分几次 push 的）

> 「无边界的字节」

---

## 1. 它解决什么问题

核心约束（一句话，最重要的那条不变式）：

> 任何时刻，buffer区的大小 不能超过 总量。

为什么需要这个约束（不限制会怎样）：

✍️ 没有这个限制可能会导致内存区一直无限制的增大，导致程序崩溃

>

它还保证字节的什么性质？（提示：先 push 的先被 pop —— 这叫什么？）

> FIFO，先进先出

---

## 2. 两个视角：Writer 和 Reader

🔍 回去看：byte_stream.hh 第 34–54 行，把两个类的公有接口逐个抄下来并写语义。

### Writer（写入端 / 生产者视角）—— 它能做哪些事？

| 函数 | 语义（我的话） | 会改数据吗？ |
|---|---|---|
| `push(data)` | 像缓冲区写加数据 | 会改buffer |
| `close()` | 这是写端完成的标志，close=ture | 会改close为true |
| `is_closed()` | 读取现在close的值 | 不会改，只是读取 |
| `available_capacity()` | 会返回现在的可写入容量 | 不会改，只是计算一下现在可写入的容量 |
| `bytes_pushed()` | 只是返回bytes_pushed_这个变量 | 不会改 |

### Reader（读取端 / 消费者视角）—— 它能做哪些事？

| 函数 | 语义（我的话） | 会改数据吗？ |
|---|---|---|
| `peek()` | 返回一个可见的buffer字符串 | 不会改数据只是看一下 |
| `pop(len)` | 取出并删除对应的长度的缓冲区内容 | 会修改缓冲区 |
| `is_finished()` | 判断现在是不是已经完成了，条件是close==true并且缓冲区为空 | 不会改，只是判断 |
| `bytes_buffered()` | 返回现有的缓冲区size | 不会改 |
| `bytes_popped()` | 返回总读取长度 | 不会改 |

⚠️ 自检探针：真正会**修改** buffer 的函数一共几个？分别是哪几个？（填完上表数一下「会改数据吗」那列）

> 3 个：pop，push，close

### 为什么要拆成两个接口（设计意图）

✍️（提示：跟「编译期职责隔离」有关）

> 什么是两个接口？

🔍 回去看：byte_stream_helpers.cc 第 45–59 行的 `writer()`。它用 `static_cast` 把数据「换面具」。
为什么这个强转是安全的？（提示：`sizeof(Writer) == sizeof(ByteStream)` + 那个 `static_assert`）

✍️ sizeof(Writer) == sizeof(ByteStream)，如果不修改write这个子类那么这个就会相等 + static_assert 会在编译时就进行判断，其实我不太会！！！？？？

>

---

## 3. ⭐ 五个量的关系（最容易混 —— 重点区）

先把每个量是「瞬时」还是「累计」标出来：

| 量 | 含义（我的话） | 瞬时 / 累计 | 只增不减？ |
|---|---|---|---|
| `capacity` | buffer的最大容量 | 瞬时 | 可增可减 |
| `available_capacity()` | 现在buffer还剩多少容量 | 瞬时 | 可增可减 |
| `bytes_pushed` | buffer总共push了多少字节 | 累计 | 只增不减 |
| `bytes_popped` | buffer总共pop了多少字节 | 累计 | 只增不减 |
| `bytes_buffered()` | 现在的buffer还有多少字节 | 瞬时 | 可增可减 |

### 两条核心等式（自己推，不要背）

等式一（守恒律）：

> `bytes_buffered()` == bytes_pushed() - bytes_poped() 

等式二（可用容量）：

> `available_capacity()` == capacity - bytes_buffered()

⚠️ 自检探针 A：`available_capacity()` 在 .cc 里写的是 `capacity_ - buffer_.size()`。
如果改成 `capacity_ - (bytes_pushed_ - bytes_popped_)`，结果会变吗？为什么？

> 不会，因为bytes_pushed_ - bytes_popped_ = buffer_.size()

### 关系验证表（自己填数字，验证守恒律每步都成立）

场景：`capacity = 5`，依次执行下列操作。每步填完，验证 `buffered == pushed - popped`。

| 操作 | buffer 内容 | bytes_pushed | bytes_popped | bytes_buffered | available_capacity | 守恒律成立？ |
|---|---|---|---|---|---|---|
| 初始 | "" | 0 | 0 | 0 | 5 | ✅ |
| `push("helloworld")` | "hello" | 5 | 0 | 5 | 0 | 成立 |
| `pop(3)` | "lo" | 5 | 3 | 2 | 3 | 成立 |
| `push("xyz")` | "loxyz" | 8 | 3 | 5 | 0 | 成立 |

⚠️ 自检探针 B：第一步 `push("helloworld")`（想推 10 个字节，但 capacity 只有 5）——
实际进去几个？
5个 
`bytes_pushed` 加几？
+5
被丢弃的字节算进 `bytes_pushed` 吗？不算
（这就是「幽灵字节」陷阱：为什么 .cc 里是 `+= actual` 而不是 `+= data.size()`）

> 

---

## 4. close / is_closed / is_finished 的语义边界

🔍 回去看：is_finished 的实现在 byte_stream.cc 第 48–51 行。

| 概念 | 语义（我的话） | 条件 |
|---|---|---|
| `close()` | 把close设置为true | 没有条件，就是设置为true，实际上的逻辑条件是写端写完 |
| `is_closed()` | 判断close是不是true | |
| `is_finished()` | 判断是不是读写都完成了 | is_closed() == true 且 bytes_bufferred == 0? |

「尾巴期」是什么？描述一个 `is_closed() == true` 但 `is_finished() == false` 的具体时刻：

✍️

>  写端结束，但是读端还没结束

⚠️ 自检探针：能不能存在「`is_finished() == true` 但写端从没 close 过」的情况？为什么？

> 不能，因为is_finished() == true的条件之一就是close == true

---

## 5. 数据结构选型

我当初用什么数据结构存 buffer？

> string（看 byte_stream.hh 第 28 行确认）

为什么选它？它的 push（尾部加）和 pop（头部删）分别对应什么操作？

✍️ 我忘记了，当时是AI帮我选的，+=可以追加，删除时erase

>

⚠️ 这个选型有没有性能隐患？（提示：从头部 `erase` 的代价是 O(?)；stress/speed test 为什么还能过？capacity 通常多大？）
—— 这一格可以先留着，等以后回头优化时再填：

> O(n)?我不记得了stress/speed test 为什么还能过？capacity 通常多大？这些是什么？

---

## 6. 易错点 / off-by-one / 踩坑记录区

（随时往这里追加。今天能想到的先写，以后撞到坑回来补。）

- 幽灵字节：____
- string_view 悬空：peek 拿到的 view 在 原string迁移 之后可能失效，因为 原string在peek 拿到的 view后迁移，peek 拿到的 view内的指针还是指向了原来string内指针指向的地址，但是可能已经被删除了。
  （延伸：SSO 短字符串时 ASan 可能失效，所以不能只靠 sanitizer。）
-
-

---

## 7. 与下一步（check1 Reassembler）的接口

ByteStream 在 check1 里扮演什么角色？（一句话）

✍️

>

Reassembler 会用 ByteStream 的哪个查询函数来判断「下游还能吃多少」？

> ____

（详细留到 check1 对话，这里只记接口关系。）

---

## 自测验收（全填完后，不看笔记，口头答这 5 题）

1. capacity 限制的是 现在buffer的容量，不是 总经过的容量。
2. `bytes_buffered == bytes_pushed() - bytes_poped()`。
3. `is_finished` 的两个条件：close == true 和 buffer.empty()。
4. peek 拿到 string_view 后，为什么不能马上 push？因为buffer是一个string字符串，push可能导致buffer增大，这个增大如果超过了buffer.总量，那么就会进行迁移，导致peek 拿到 string_view内的指针失效，这个可能导致出现乱码或者直接访问了未定义的地址而出错
5. 为什么 push 累计要 `+= actual` 而不是 `+= data.size()`？
因为actual = min（data.size，剩余可写容量），每次的push都不一定能吧data.size全写入，要考虑剩余容量所以需要+= actual
