# Am-241 γ (59.5 keV) 逐通道增益标定可行性研究

目标：判断能否用 Am-241 的 59.5 keV γ 对 98 芯片读出板的灵敏通道做逐通道增益标定，
并给出能量核算、正演谱和可行性结论。

## 几何与源

- **读出板**：98 芯片（49 灵敏 + 49 veto），12 mm 最近邻六角点阵。
  布局来自 `nvdex-tpc-readout-analysis/examples/enc_20261113/channels.csv`，
  导出为 `../alpha_array/config/array_98ch.csv`（第 1 行在顶部，与 9734 实测图一致）。
- **场笼**：13 cm 漂移高度，置于 60 cm 大氣腔内；场笼内气体为 Ar 或 Xe（1 bar）。
- **源**：Am-241 59.5 keV γ，位置在阴极背面（场笼外）1 mm，**在气体中**。
  两种位置：偏心源 = ADC5-IN4 与 ADC5-IN3 中点；中心源 = 灵敏区质心。
  向下半球（2π）各向同性发射，无狭缝。
- 坐标：z 轴竖直向上，读出平面在 z = −300 mm（圆柱底面），阴极在 z = −170 mm，
  场笼 = z ∈ [−300, −170] mm，其上方为外层气体。

## 物理配置（Geant4）

| 项 | 值 |
| --- | --- |
| 版本 | Geant4 **11.4.0** |
| EM 模型 | **G4EmStandardPhysics_option4**（Livermore：EPICS2014 光电、Klein–Nishina 康普顿、Rayleigh） |
| 原子退激 | **G4AtomicDeexcitation**（荧光 + Auger，option4 默认开启） |
| 产生阈值 | γ/e± 0.1 mm、质子 1 mm（`prod_cut_*_mm` 可改） |
| 电场 | 本研究关闭（`field_strength_V_per_cm: 0`） |
| 附加物理 | `G4DecayPhysics` |

**G4EMLOW 只是数据集**；上述模型与数据版本才是本研究实际使用的物理模型。

## 正演模型（edep → 电荷）

`forward_response.py`：
1. **W/Fano**：n_e = edep_eV / W（Xe: W = 21.9 eV，F = 0.17），
   高斯涨落 σ = √(F·mean)，总方差 Var(N) = F·N。
2. **无电子吸附**（存活率 = 1）。
3. **横向扩散**：200 V/cm，σ_T = D_T·√(漂移 cm)，D_T 默认 1.3 mm/√cm，
   可扫描（0/0.5/0.8/1.0/1.3）。
4. **芯片归因**：XY 最近中心（7 mm 接收半径），增益统一 **124 mV/fC**，无实测 ENC。

**全能计数窗口**：严格口径 **58–62 keV（或 keV-eq）**；宽松口径 55–62 已弃用
（含 L 壳逃逸，约 13%–73% 视扩散而定）。

## 主要结果

### 相互作用与过程分解（10⁷ 初级光子）

| 指标 | Ar @1 bar | Xe @1 bar |
| --- | --- | --- |
| 相互作用率（偏心 / 中心） | 0.86% / 0.94% | 38.5% / 41.9% |
| 首次相互作用为光电效应 | 69% | 98.6% |
| 全能事件（仅场笼内） | ~45% | ~7.6% |

Xe 的 ~26/30/55/59.5 keV 结构：Kβ 逃逸（26）、Kα 逃逸（30）、L 壳逃逸（55）、全能峰（59.5）。
Ar 连续谱主因是**光电子跨芯片分摊 + 光电子飞出场笼**，不是 Compton 主导。

### 逐通道全能计数（严格 58–62 keV）

| 配置 | 总数 | 每通道中位 / 最少 / 最多 |
| --- | --- | --- |
| Xe-central 10⁷ | 61187 | 428 / 189 / 5163 |
| Ar-central 10⁷ | 23 | 0 / 0 / 3 |
| Xe-central D_T=0.5（10⁷） | 13972 | 108 / 53 / 920 |
| Xe-central D_T=0.8（10⁷） | 2552 | 24 / 10 / 59 |
| Xe-central D_T=0.8（2×10⁷） | 4660 | 47 / 25 / 121 |

### 扩散扫描（严格全能总数）

D_T=0 → 53363；0.5 → 13972；0.8 → 2552；1.0 → 1074；1.3 → 430。
D_T 是可行性的主要控制变量：13 cm 漂移下的扩散决定单芯片能否收满能量。

## 结论

- **Xe@1bar + 中心源：可行**。纯 edep 下每通道 ≥189 例（精度 ~0.1%）；
  D_T=0.5 时 10⁷ 可行（中位 108）；D_T=0.8 时 2×10⁷（约 50 min @1 µCi）
  最差通道 25 例，统计精度 ~0.2%，满足 Qββ 1% 目标对增益标定 ~0.3–0.5% 的要求。
- **Ar@1bar：逐通道标定不可行**（光电子跨芯片分摊，全板仅 23 例严格全能）。
  可做簇和的绝对能量锚。
- **拟合必须包含 Kα/Kβ/L 逃逸结构**，不能只用 55–62 计数窗口；
  正式拟合取峰位 ±2–3σ。
- 达到 D_T=0.8 需 E/N ≈ 450 V/(cm·bar)（1 bar 时 ~450 V/cm，0.8 bar 时 ~360 V/cm）；
  D_T=0.5 需 E/N ≈ 1000 V/(cm·bar)（1 bar 时 ~1 kV/cm，0.8 bar 时 ~800 V/cm）。

## 文件与复现

- `config/`：Ar/Xe 的 1 bar 配置（偏心、中心）
- `analyze_gamma.py`：edep 分析（能谱、计数图、3D 事例）
- `forward_response.py`：W/Fano + 横向扩散 + 芯片电荷正演
- `draw_layout.py`、`draw_process_schematics.py`：布局图与过程示意图
- `figures/`：全部结果图

```bash
# 在装有 Geant4 的主机上（此处 imp-ubuntu）
source /home/wuwenjie/software/install/geant4/11.4.0-gcc13-release-mt/bin/geant4.sh
cd /home/wuwenjie/gasTPC/build && make -j16
d=/home/wuwenjie/Data/gasTPC-crosscheck/gamma-241Am/Xe-central
./gasTPC --config $d/config.json $d/run.mac   # run.mac 内写 /run/beamOn N
cd ../studies/gamma_241Am
/home/wuwenjie/nvdex-tpc-readout-analysis/.venv/bin/python analyze_gamma.py $d \
  --layout ../alpha_array/config/array_98ch.csv
/home/wuwenjie/nvdex-tpc-readout-analysis/.venv/bin/python forward_response.py $d \
  --layout ../alpha_array/config/array_98ch.csv --diff-mm 0.8
```

数据输出在 `/home/wuwenjie/Data/gasTPC-crosscheck/gamma-241Am/`（不入 Git）。
