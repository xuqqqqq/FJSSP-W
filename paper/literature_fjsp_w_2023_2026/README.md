# 近三年 FJSP-W / DRCFJSP 文献下载清单

本文件夹用于整理 2023--2026 年左右与 FJSP-W 直接相关的论文。文献里这个问题常用的名字不止一个：

- `FJSP-W` / `FJSSP-W`: Flexible Job Shop Scheduling Problem with Worker Flexibility。
- `DRCFJSP` / `DRC-FJSSP`: Dual-Resource Constrained Flexible Job Shop Scheduling Problem。
- `worker-constrained FJSP`: 强调工人约束、工人异质性、熟练度、疲劳、无聊、缺勤等人的因素。

第三方 PDF 已保存在本地，但默认不纳入 Git 跟踪；仓库只提交这个索引和下载来源。

## 建议阅读顺序

1. `2025_Hutter_Comprehensive_Benchmarking_FJSSP-W_arXiv.pdf`
   - 最贴近比赛本身，讲 FJSSP-W benchmark/environment。
   - 可用于论文背景里解释“为什么标准 FJSP 不够，为什么要加 worker flexibility”。

2. `2025_Li_CEAM_CP_DRCFJSP_CSMS.pdf`
   - 2025 年开放获取论文，题名为 `Novel Hybrid Algorithm of Cooperative Evolutionary Algorithm and Constraint Programming for Dual Resource Constrained Flexible Job Shop Scheduling Problems`。
   - 明确把 DRCFJSP 拆成三件事：operation sequencing、machine assignment、worker assignment。
   - 它的 CEAM+CP 两阶段结构可以作为我们“AWLS+精确可行性修复/局部优化”的对照。

3. `2025_Wang_Dual_Resource_FJSP_Worker_Proficiency_author_manuscript.pdf`
   - 题名为 `Dual-resource flexible job shop scheduling considering worker proficiency differences`。
   - 适合用来讲工人不只是第二类资源，而是会有 proficiency/skill 差异。

4. `2024_Magalhaes_xBTF_Dual_Resource_Flexible_Job_Shop_IPMU.pdf`
   - 题名为 `Dual Resource Flexible Job Shop Scheduling Problems: The xBTF Algorithm`。
   - 一个很快的构造式启发式，适合和我们的局部搜索对比：它强调超快 rescheduling，我们强调在比赛静态算例里继续压 makespan。

5. `2024_Barak_DRCFJSP_Sequence_Dependent_Setup_Time_ExpertSystems_author_manuscript.pdf`
   - 题名为 `Dual resource constrained flexible job shop scheduling with sequence-dependent setup time`。
   - 可用于背景扩展：近年 DRCFJSP 不只是加工人，还常叠加 setup time、uncertainty、multi-objective 等复杂现实因素。

6. `2025_Zeng_Dual_Resource_Scheduling_FBI_Mathematics.pdf`
   - 题名为 `Dual-Resource Scheduling with Improved Forensic-Based Investigation Algorithm in Smart Manufacturing`。
   - 适合补充“智能制造下双资源调度算法大量使用群智能/元启发式”的背景。

7. `2024_TechScience_DFJSP_Dual_Resource_Constraints_CMES.pdf`
   - 题名为 `Energy-Saving Distributed Flexible Job Shop Scheduling Optimization with Dual Resource Constraints Based on Integrated Q-Learning Multi-Objective Grey Wolf Optimizer`。
   - 分布式、节能、多目标版本，适合作为扩展背景，不是我们比赛主线。

8. `2023_IEOM_DRCFJSP_DES_Proceedings.pdf`
   - 题名为 `Analysis of Priority Decision Rules Using MCDM Approach for a Dual-Resource Constrained Flexible Job Shop Scheduling by Simulation Method`。
   - 可作为调度规则/仿真背景，算法深度弱于前几篇。

9. `2026_Yuraszeck_Multiresource_FJSP_Early_Resource_Release_Mathematics.pdf`
   - 多资源 FJSP，不是严格 FJSP-W，但对“机器与工人同时占用/释放”这类资源建模有启发。

10. `2022_Abels_Memetic_RL_Sociotechnical_DRC_FJSSP_arXiv_nearby.pdf`
    - 稍早于三年窗口，但和 sociotechnical / worker-aware production scheduling 关系很近。

## 暂时没有公开 PDF 的重点论文

这些我已经定位到入口，但自动下载不是开放 PDF，后续可以用学校机构访问：

- `Multi-agent model-based intensification-driven tabu search for solving the dual-resource constrained flexible job shop scheduling`
  - 链接：https://www.tandfonline.com/doi/full/10.1080/24751839.2024.2425478
  - 这篇和我们最像，因为也是 tabu-search 风格。

- `A reinforcement learning enhanced memetic algorithm for multi-objective flexible job shop scheduling toward Industry 5.0`
  - 链接：https://www.tandfonline.com/doi/abs/10.1080/00207543.2024.2357740
  - 重点是 worker flexibility、learning effect、fuzzy processing time、多目标。

- `A new boredom-aware dual-resource constrained flexible job shop scheduling problem using a two-stage multi-objective particle swarm optimization algorithm`
  - 链接：https://www.sciencedirect.com/science/article/pii/S0020025523007260
  - 重点是 worker boredom，是人因建模里很有代表性的一篇。

- `Dynamic scheduling for dual-resource constrained flexible job-shop via semantic-aware graph modelling and deep reinforcement learning`
  - 链接：https://www.sciencedirect.com/science/article/pii/S095070512501233X
  - 重点是知识图谱 + DRL + 动态双资源 FJSP。

- `From human-related to human-centric: A review of shop floor scheduling problem under Industry 5.0`
  - 链接：https://www.sciencedirect.com/science/article/pii/S0278612525001785
  - 适合重写论文背景时引用，能把 FJSP-W 放到 Industry 5.0/human-centric scheduling 里。

- `A multitasking workforce-constrained flexible job shop scheduling problem: An application from a real-world workshop`
  - 链接：https://www.sciencedirect.com/science/article/pii/S0278612525002304
  - HUST 相关真实车间应用，值得你用学校权限看一下。

## 对我们论文背景的启发

论文背景应该按这条线展开，而不是直接跳到算法：

1. 标准 FJSP 只决定机器选择和工序排序，隐含假设是机器资源决定一切。
2. 真实车间里机器通常需要工人操作，工人数量有限、技能不同，并且一个工人不能同时操作无限多台机器。
3. 因此 FJSP-W / DRCFJSP 引入第三个决策：worker assignment，同时增加工人资源冲突约束。
4. 近三年研究进一步把 worker flexibility 扩展到 proficiency、learning/forgetting、fatigue/boredom、absence、multi-worker collaboration、dynamic rescheduling。
5. 本比赛的 FJSSP-W benchmark 抽象掉一部分人因细节，保留最核心的机器-工人双资源可行性，因此很适合验证“把强 FJSP 局部搜索改造成 worker-aware local search”的价值。
