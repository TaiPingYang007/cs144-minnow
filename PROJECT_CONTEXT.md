# PROJECT_CONTEXT - cs144

## 项目定位

- 正式仓库：`/home/taipingyang007/projects/cpp_project/05_Stanford-CS144-Computer_Network_Systems/cs144-minnow`
- GitHub 正式仓库：`TaiPingYang007/cs144-minnow`
- 定位：使用单个正式仓库持续推进 CS144 labs，不再依赖多仓库或 starter 分支路线

## 仓库与关键路径

- 关键文件：
  - `src/reassembler.hh`
  - `src/reassembler.cc`
  - `tests/reassembler_single.cc`
  - `tests/reassembler_seq.cc`
  - `tests/reassembler_holes.cc`
  - `src/byte_stream.cc`
  - `src/tcp_receiver.*`
- 课程材料：
  - `handouts/check0.pdf` 到 `handouts/check7.pdf`

## 恢复入口

- 当前主恢复入口是 `check1: Reassembler`
- 先读 `reassembler.hh / reassembler.cc`
- 再读 `reassembler_single / reassembler_seq / reassembler_holes`
- 然后确定状态设计与重叠 / 窗口处理思路
- 动态推进、当前阻塞、近期结论与续接点优先进项目记忆

## 稳定约束

- 只维护一个正式仓库，不再纠结 starter 分支路线
- 远端当前只有 `main` 分支，后续 labs 不存在可直接切换的 starter 分支
- 业务 `.cc` 文件保留 starter 状态，不把别人的完整答案原样带入正式仓库
- 当前不扩展到 `check3+`
