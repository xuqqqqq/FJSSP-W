# FJSP-W / DRCFJSP 近期文献中文读书笔记

范围：本笔记覆盖本文件夹中已下载的 10 篇 PDF。这里不做逐字翻译，而做“写论文和改算法能直接用”的归纳：每篇包括一句话、问题建模、算法、实验结论、对我们工作的启发。

## 总体脉络

FJSP-W 在文献里通常叫 `DRCFJSP` 或 `DRC-FJSSP`，核心变化是：标准 FJSP 只有“工序排序 + 机器选择”，而 FJSP-W/DRCFJSP 还要同时决定“工人选择”，并满足机器和工人两类资源都不能冲突。

近三年文献可以分成五条线：

- 基准与可复现实验：Hutter et al. 2025 给出 FJSSP-W benchmark/environment，这是我们比赛最直接的背景。
- 双资源元启发式：CEAM-CP、xBTF、Q-MOGWO、TSFBI、MOIWO 等都在做机器-工人双资源下的编码、解码和邻域/进化。
- 人因建模：工人熟练度、工作时长、学习/遗忘、疲劳、无聊、缺勤等被逐步加入模型。
- 多目标与现实约束：除了 makespan，还常考虑能耗、人工成本、迟早交惩罚、setup time、机器负载均衡。
- 精确/混合方法：CP/MILP 常用于小规模验证或作为二阶段局部强化，大规模仍依赖启发式/元启发式。

对我们的论文背景，最好的叙事不是“我们把 AWLS 改了一下”，而是：

1. 标准 FJSP 忽略工人资源，难以表示真实人机协同车间。
2. FJSP-W/DRCFJSP 把每道工序扩展成 operation-machine-worker 三元选择。
3. 这会带来第三类决策变量和新的资源冲突边，因此析取图、邻域动作、可行性判断都必须 worker-aware。
4. 近年很多工作把问题做得更复杂，但比赛保留的是最核心的双资源可行性和 makespan 目标，因此适合验证“强 FJSP 局部搜索如何升级成双资源局部搜索”。

---

## 1. Hutter et al. 2025

文件：`2025_Hutter_Comprehensive_Benchmarking_FJSSP-W_arXiv.pdf`

题名：A Benchmarking Environment for Worker Flexibility in Flexible Job Shop Scheduling Problems

一句话：这是最贴近比赛的论文，核心贡献是把 402 个常用 FJSSP 实例系统扩展成 FJSSP-W，并提供统一 benchmark 环境和 baseline。

问题建模：

- 标准 FJSSP 中，每个 operation 选择一台可加工机器，并满足工序前后约束和机器互斥。
- FJSSP-W 中，每台机器加工时还要求一个工人在场，且不同工人可能导致不同加工时间。
- 因此 operation 的可选项从 `(machine, processing time)` 变为 `(machine, worker, processing time)`。
- 论文默认用 makespan 作为当前环境主要评价指标，但环境设计上可扩展到其他指标。

基准生成：

- 从 402 个传统 FJSSP benchmark 出发。
- 默认工人数设为 `1.5 * machine_count`。
- 对每个 operation-machine pair，随机生成可操作工人集合。
- worker-specific processing time 在原始加工时间附近随机扰动，默认是原时间的 0.9 到 1.1 倍。

实验结论：

- 对比 Greedy、MILP、CP、GA。
- 20 分钟时间限制下，CP 和 GA 明显优于 MILP 和 Greedy。
- MILP 在大实例上容易变量/约束过多，出现内存或时间问题。
- 作者强调固定 benchmark、固定参数和公开结果的重要性，否则算法之间很难公平比较。

对我们的启发：

- 我们论文背景必须引用这篇，因为比赛实例格式和研究动机基本来自它。
- 我们的求解器应强调“可复现 benchmark 结果”，比如固定随机种子、时间限制、硬件、提交 CSV。
- 算法解释里要明确：AWLS 原本的析取图只考虑 machine arcs；FJSSP-W 需要同时处理 worker arcs。

---

## 2. Li et al. 2025

文件：`2025_Li_CEAM_CP_DRCFJSP_CSMS.pdf`

题名：Novel Hybrid Algorithm of Cooperative Evolutionary Algorithm and Constraint Programming for Dual Resource Constrained Flexible Job Shop Scheduling Problems

一句话：这篇是非常适合对标我们工作的 DRCFJSP 算法论文，它把问题明确拆成三层编码：工序序列、机器选择、工人选择，并用 CEAM + CP 两阶段求解。

问题建模：

- DRCFJSP 目标是最小化 makespan。
- 三个子问题：machine allocation、worker allocation、operation sequencing。
- 每道工序必须选择一台机器和一个可操作该机器的工人。
- 同一机器上的操作不能重叠，同一工人负责的操作也不能重叠。

算法：

- 三层编码：`OS` 表示工序顺序，`MS` 表示机器选择，`WS` 表示工人选择。
- CEAM 阶段用三个种群分别对应三个子问题，协同进化。
- full active decoding 用插空和右移进一步压缩 makespan。
- CP 阶段以 CEAM 找到的好解为起点，用 CP 做二次强化。

实验结论：

- 在 13 个 benchmark 上测试。
- CEAM-CP 优于单交叉/多交叉 CEAM。
- 论文报告改进了 13 个实例中的 9 个 best-known solutions。
- 给出的改进值包括 DFJSP03=4、DFJSP04=31、DFJSP07=47、DFJSP08=154、DFJSP09=45、DFJSP10=124、DFJSP11=459、DFJSP12=279、DFJSP13=174。

对我们的启发：

- 这篇可以作为我们“编码/邻域设计”的直接相关工作。
- 它的三层编码对应我们代码中的 operation sequence、machine assignment、worker assignment。
- 它用 CP 做后处理，说明 hybrid exact/local improvement 是合理路线；我们也可以写成“AWLS generates high-quality incumbent, then exact/repair-style schedule evaluation guarantees feasibility”。
- full active decoding 对我们很有启发：worker-aware 插空解码比简单 earliest-start 更关键。

---

## 3. Wang et al. 2025

文件：`2025_Wang_Dual_Resource_FJSP_Worker_Proficiency_author_manuscript.pdf`

题名：Dual-resource flexible job shop scheduling considering worker proficiency differences

一句话：这篇重点不是新 benchmark，而是说明“工人不是同质资源”，不同熟练度会影响准备时间，从而改变最优调度。

问题建模：

- 关注 DRCFJSP 中 worker proficiency differences。
- 加工时间被拆成机器加工时间和工人相关 preparation/setup time。
- 工人熟练度越高，某些准备过程可能越短。
- 模型目标是最小化总完成时间/最大完成时间。

算法：

- 使用 Migratory Bird Optimization（MBO，候鸟迁徙优化）。
- 解由工序顺序、机器选择、工人选择组成。
- 通过 lead bird 和 following birds 的邻域演化搜索更优解。
- 对照算法使用 GA。

实验结论：

- 大案例中，考虑不同熟练度时，MBO 得到 makespan 290；假设工人同质时为 299。
- GA 下相应结果为 331 和 334。
- 小案例中，MBO 分别为 129 和 134，GA 为 159 和 166。
- 结论是：显式考虑工人熟练度能缩短完成时间，且 MBO 优于 GA。

对我们的启发：

- 比赛的 FJSSP-W 主要把 worker reflected processing time 编进实例，没显式建模年龄/经验，但本质也是 worker-dependent processing time。
- 论文背景可以用这篇说明：工人选择不是附属变量，而会直接影响工序持续时间和最终 makespan。
- 如果之后继续优化算法，可加入 worker load/proficiency-aware 初始化，而不是只选最短加工时间。

---

## 4. Magalhaes et al. 2024

文件：`2024_Magalhaes_xBTF_Dual_Resource_Flexible_Job_Shop_IPMU.pdf`

题名：Dual Resource Flexible Job Shop Scheduling Problems: The xBTF Algorithm

一句话：这篇提出一个极快的构造式启发式 xBTF，核心是调度时优先处理“最大威胁”，并在资源分配时加入机器/工人预期负载惩罚。

问题建模：

- DRC-FJSSP 中每个 operation 需要一个 eligible worker-machine pair。
- 每台机器同一时间只能处理一个 operation。
- 每个工人同一时间只能操作一台机器。
- 目标是最小化 makespan。

算法：

- 继承 Biggest Threat First（BTF）思想。
- 先构造 blacklist，按威胁程度排序操作。
- xBTF 改进点是在分配资源时估计每个工人/机器的预期工作负载。
- 对过载倾向资源加 penalty，避免早期贪心把长工序堆到少数资源上。

实验结论：

- 用 MK1-10 benchmark，每个实例生成 100 个样本。
- 对比 KGFOA、BTF、xBTF。
- xBTF 在复杂高资源负载实例上优于 BTF，并整体显著优于 KGFOA。
- xBTF 运行时间通常小于 0.60 秒，作者认为适合动态重调度。

对我们的启发：

- 我们可以借鉴“资源负载惩罚”做初始化或快速修复：不仅看 earliest feasible time，也看 worker/machine 后续负载。
- 对比赛静态实例，xBTF 这类算法可作为快速初解；AWLS 再做深度局部搜索。
- 论文里可以把它作为“fast constructive heuristic”相关工作，和我们的 local search 路线区分开。

---

## 5. Barak et al. 2024

文件：`2024_Barak_DRCFJSP_Sequence_Dependent_Setup_Time_ExpertSystems_author_manuscript.pdf`

题名：Dual resource constrained flexible job shop scheduling with sequence-dependent setup time

一句话：这篇把 DRCFJSP 和 sequence-dependent setup time 结合，目标不再只是 makespan，而是三目标：迟早交、工人空闲/setup 成本、机器负载。

问题建模：

- 每道工序由 machine-operator 组合执行。
- 不同 machine-operator 组合可以有不同处理时间。
- setup time 依赖前后两个操作的顺序，即 sequence-dependent setup time。
- 工人数量少于机器数量，工人多技能，但同一时间只能处理一个任务。

目标：

- 最小化 earliness/tardiness penalty。
- 最小化 operator idleness cost 和 setup cost。
- 最小化最大机器负载。

算法：

- 小规模问题用 epsilon-constraint method 求精确 Pareto front。
- 大规模问题用 MOIWO（multi-objective invasive weed optimization）。
- MOIWO 中加入 fuzzy sorting competitive mechanism。
- 对比 NSGA-II 和 MOPSO。

实验结论：

- MOIWO 在小规模上接近 epsilon-constraint。
- 对比 NSGA-II，MOIWO 成功率约 90.83%，另有 4.17% 表现相近。
- 对比 MOPSO，MOIWO 成功率约 84.17%，另有 9.17% 表现相近。
- 作者强调不同算法在成本、负载均衡、交期稳定之间有不同偏好。

对我们的启发：

- 比赛没有 setup time，但这篇说明工人约束经常和 setup/operator cost 一起出现，是很好的“现实复杂性背景”。
- 我们如果只优化 makespan，要明确这是比赛目标，不代表 FJSP-W 现实目标只有 makespan。
- 这篇还支持我们在论文中说：双资源扩展会显著增加约束结构，exact method 只适合小规模，大规模需要启发式/局部搜索。

---

## 6. Zhang et al. 2024

文件：`2024_TechScience_DFJSP_Dual_Resource_Constraints_CMES.pdf`

题名：Energy-Saving Distributed Flexible Job Shop Scheduling Optimization with Dual Resource Constraints Based on Integrated Q-Learning Multi-Objective Grey Wolf Optimizer

一句话：这篇研究分布式 FJSP + 工人约束 + 能耗目标，提出 Q-learning 增强的多目标灰狼优化算法。

问题建模：

- 多工厂 distributed FJSP。
- 每个 job 选择工厂，每道工序选择机器和工人。
- 工人有不同技能，可操作不同机器。
- 目标是同时最小化 makespan 和总能耗。

算法：

- 四层编码：operation sequence、factory selection、machine selection、worker selection。
- active decoding 依赖 machine-worker 公共空闲时间。
- Q-MOGWO 基于 multi-objective grey wolf optimizer。
- Q-learning 自适应选择 critical factory local search 策略。

实验结论：

- 扩展了 45 个测试实例，工厂数为 2、3、4。
- 对比 MOEA/D、MOGWO、NSGA-II、MA。
- 指标包括 IGD、Spread、HV。
- Q-MOGWO 在 IGD 和 HV 上整体优于对比算法，尤其大规模实例更明显。

对我们的启发：

- `public idle time` 这个概念很重要：FJSP-W 中一个 operation 的可插入位置必须同时满足机器和工人的共同空闲窗口。
- 我们的邻域评价/解码要避免只看机器时间线，否则会产生不可行或估计错误。
- Q-learning 自适应邻域说明“根据当前解状态选择邻域”是近期趋势；AWLS 里可以考虑 worker critical path 状态驱动的邻域选择。

---

## 7. Zeng et al. 2025

文件：`2025_Zeng_Dual_Resource_Scheduling_FBI_Mathematics.pdf`

题名：Dual-Resource Scheduling with Improved Forensic-Based Investigation Algorithm in Smart Manufacturing

一句话：这篇把 DRCFJSP 做成双目标：makespan 和人工成本，并考虑工人弹性工时；算法是两阶段 TSFBI + AHP 决策。

问题建模：

- 每道工序选择机器和工人。
- 工人有标准工时、最低/最高工作时长、加班成本和基础工资。
- 目标 1 是最小化 makespan。
- 目标 2 是最小化总人工成本，包括标准工时成本、加班成本、基础工资。

算法：

- 第一阶段用 improved multi-objective FBI 算法生成 Pareto front。
- 用 hybrid real/integer encoding-decoding 映射调度解。
- 用 fast non-dominated sorting 保持 Pareto 集。
- 第二阶段用 AHP 根据管理者偏好从 Pareto front 中选一个满意解。

实验结论：

- 扩展了 51 个测试案例，来源包括 la01-la40、BRData、Mk01-Mk10 和一个简单案例。
- 第一组实验用 Gurobi 验证模型正确性。
- 第二组实验对比 NSGA-II，TSFBI 在 C-metric、SM、HVR 等指标上更好。
- 第三组实验说明 AHP 可以按偏好从 Pareto 集中选解。

对我们的启发：

- 比赛只看 makespan，但真实 FJSP-W 常同时看人工成本。
- 我们论文可以把“human-centric manufacturing”讲清楚：工人不是只增加约束，还引入劳动成本、工时公平、可靠性等管理因素。
- 如果未来扩展赛道/论文，可以把 AWLS 从单目标改成 multi-objective 或 lexicographic objective。

---

## 8. IEOM 2023 Proceedings

文件：`2023_IEOM_DRCFJSP_DES_Proceedings.pdf`

题名：Analysis of Priority Decision Rules Using MCDM Approach for a Dual-Resource Constrained Flexible Job Shop Scheduling by Simulation Method

一句话：这篇不是强优化算法，而是用离散事件仿真比较 DRCFJSP 中不同 dispatching/priority rules。

问题建模：

- DRCFJSP 中工序需要机器和工人两类资源。
- 评价规则考虑 demand、due date、cycle time、number of operations、setup time 等因素。
- 用 DES 模拟不同 priority rule 下的车间表现。

方法：

- 比较 Composite Dispatching Rules（CDR）和 MCDM-based rules。
- MCDM 包括 PSI、PIV。
- 规则权重由 Fuzzy SWARA 得到。
- 案例包括小规模数值例和大规模真实问题：114 jobs、28 machines、23 workers。

实验结论：

- 综合考虑 processing time、least work remaining、earliest due date 的 CDR 整体最好。
- 大规模案例中 PSI-based rule 排第二。
- MCDM 的优势是可以按专家经验修改评价标准和权重。

对我们的启发：

- 这篇可作为“快速调度规则/仿真方法”的背景。
- 对我们代码来说，dispatching rules 可以做初解或 fallback，不适合直接替代 AWLS。
- 论文里可以用它说明：工业场景需要简单稳健规则，但比赛追求 best-known solution 时需要更强搜索。

---

## 9. Yuraszeck et al. 2026

文件：`2026_Yuraszeck_Multiresource_FJSP_Early_Resource_Release_Mathematics.pdf`

题名：The Multiresource Flexible Job-Shop Scheduling Problem with Early Resource Release

一句话：这篇不严格是 FJSP-W，但很适合帮助理解“多资源工序是否必须同时占用并同时释放”的建模问题。

问题建模：

- 研究 multiresource FJSP。
- 标准 simultaneous occupation 假设：工序开始时所有资源都可用，并且资源同时占用、同时释放。
- 本文放宽这个假设：工序开始时资源同时到位，但不同资源可以占用不同持续时间，即 early resource release。
- 用 MMRCPSP 形式建立 CP 模型。

实验结论：

- 测试 65 个 MRFJSSP 实例。
- 证明 44 个实例达到最优。
- 平均 optimality gap 为 9.37%。
- 在一些实例上，early release 可明显降低 makespan，尤其资源少、灵活性低时收益更大。

对我们的启发：

- 当前比赛 FJSSP-W 基本采用 simultaneous occupation：一个 operation 加工期间同时占用 machine 和 worker。
- 但现实里工人可能只参与 setup 或 loading，不一定全程占用。
- 我们论文可以把比赛模型说清楚：本文处理的是“工人全程占用”的 FJSSP-W；partial worker occupancy 是未来扩展。

---

## 10. Grumbach et al. 2022/2023

文件：`2022_Abels_Memetic_RL_Sociotechnical_DRC_FJSSP_arXiv_nearby.pdf`

题名：A Memetic Algorithm with Reinforcement Learning for Sociotechnical Production Scheduling

一句话：这篇稍早于三年窗口，但很有价值，因为它从真实工业数据出发，把 DRC-FJSSP 做成 sociotechnical scheduling，并用 memetic algorithm + DRL 改进搜索。

问题建模：

- 模型比比赛复杂很多：柔性机器、柔性工人、工人能力、setup/processing operations、material arrival、BOM/DAG job paths、sequence-dependent setup times、部分自动化任务。
- 支持机器 assignment、worker assignment、task sequencing 同时优化。
- 目标示例是 makespan 和 total tardiness 的加权组合。
- 用 DES 评价排程。

算法：

- memetic algorithm 框架。
- 用 DRL agent 替代随机邻域生成中的部分 sequencing/assignment 决策。
- DES 用于生成和评价 schedule。
- 支持并行计算，作者定位在 reactive/dynamic scheduling。

实验结论：

- 使用两个 synthetic datasets 和六个 real-world datasets。
- 在真实数据上，DRL 注入后比随机 metaheuristic 更快找到更可靠的局部最优。
- 前 10 代提升最大，说明 DRL 对早期搜索方向很有帮助。
- 作者也承认 DRL 超参数和泛化仍是问题。

对我们的启发：

- 这篇能帮我们把 AWLS 放在“memetic/local search + learning-enhanced scheduling”的脉络里。
- 当前我们先做手工 worker-aware neighborhood，未来可以学习邻域选择或移动评分。
- 它也支持一个观点：复杂真实模型中 exact solver 不现实，仿真/启发式/局部搜索仍有生命力。

---

## 重点结论：我们论文背景应该怎么改

建议把中文论文背景改成下面这个逻辑：

1. 从制造调度说起：JSP/FJSP 是经典模型，FJSP 允许一项工序在多台机器上选择加工，目标常为最小化 makespan。
2. 说明标准 FJSP 缺口：真实车间里机器往往必须由工人操作，工人数量、技能、可用性、工时都会限制调度。
3. 引入 FJSP-W/DRCFJSP：每道工序必须同时选择机器和工人，约束从单资源互斥扩展为机器-工人双资源互斥，决策空间从两层扩展到三层。
4. 说明近期趋势：benchmark 环境、双资源元启发式、CP/混合方法、多目标、人因建模、动态调度都在快速发展。
5. 聚焦比赛：本比赛抽象出最核心的 worker flexibility 和 makespan 优化，使得我们可以集中研究强 FJSP 局部搜索如何升级为 worker-aware local search。
6. 引出本文方法：在 AWLS 原有 critical-path / disjunctive-graph / tabu-search 框架上，引入 worker arcs、operation-machine-worker 三元移动、双资源可行性判断和 worker-aware schedule evaluation。

## 对我们算法优化最有用的点

- 初解：借鉴 xBTF 的 worker/machine expected workload penalty。
- 解码：借鉴 CEAM-CP 和 Q-MOGWO 的 machine-worker public idle time 插空。
- 邻域：从只动机器块，扩展到机器关键块 + 工人关键块。
- 评价：近似评价必须同时维护机器边和工人边，否则很容易低估/误判 move。
- 混合强化：可以考虑最后阶段用 CP 或局部 exact repair 对小瓶颈子图做 intensification。
- 论文定位：我们不是简单把 FJSP 加个 worker 字段，而是在图结构、邻域动作、可行性判定上完整升级。
