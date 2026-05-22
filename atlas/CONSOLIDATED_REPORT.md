# MiceCam v2 — 语义审计 + 需求符合性综合报告

> 生成日期: 2026-05-22  
> 基线: `atlas/` 5 份模块审计 + `specs/001~007` 逐 FR/defer 对照  
> 审计范围: 59 源文件, ~7385 行 C++20

---

## 一、总览

| 维度 | 数据 |
|------|------|
| 总源文件 | 59 (21 仅头文件 + 38 对 .h/.cpp) |
| 总行数 | ~7385 |
| Atlas 产出 | 5 份模块审计 + 1 份项目审计, 共 3694 行 |
| Spec 覆盖 | 001~007, 7 份 spec 文件, 总计 ~1500 行需求 |
| FR 总量 | 001:15 + 002:11 + 003:24 + 004:28 + 005:20 + 006:17 + 007:35 = **150** |
| 审计评分 | 架构 8/10, 线程安全 4/10, 测试覆盖 3/10, **综合 6/10** |

---

## 二、模块级审计结论

### 2.1 domain/ 层 (20 文件, 985 行)

**评分: HEALTHY**。20 个纯值类型 + 6 个含逻辑类型, 架构定位准确。

| # | 发现 | 风险 | 优先级 |
|---|------|------|--------|
| F1 | `TimestampEngine` 无初始化守卫 — `populate()` 在 `capture_wall_anchor()` 前调用返回无效时间戳 | 中 | P1 |
| F2 | `PluginRegistry::get_external_plugins()` 返回裸指针 — vector realloc 后悬空 | 高 | P1 |
| F3 | `PluginManifest::validate()` 不检查 `id` 格式 (路径遍历风险) | 中 | P2 |
| F4 | `SessionMetadata` 缺 FR-13 的 `plugin_id`/`plugin_version`/`capability_snapshot` 字段 | 中 | P2 |
| F5 | `PluginErrorRegistry::get()` 对未知 code 抛异常, 不符 FR-12 结构化错误原则 | 低 | P2 |
| F6 | `discover_all()` 无 try-catch, 单个 enumerator 异常阻断全局发现 | 低 | P3 |
| F7 | `to_json`/`from_json` 往返不对称 — 空字段省略后反序列化丢失 | 低 | P3 |

### 2.2 pipeline/ 层 (7 文件, ~1022 行)

**评分: 良好 (7/10)**。职责分离清晰, 5 个接口支持 mock, 崩溃恢复路径完整。

| # | 发现 | 风险 | 优先级 |
|---|------|------|--------|
| R1 | `push_frame()` 持 mutex_ 范围过大 — 锁内执行转码+文件IO, 高帧率瓶颈 | 中 | P0 |
| R2 | `start()` 中途失败不清理已创建的 StreamPipeline 和目录 | 高 | P0 |
| R3 | `snapshot()` 作为 public 方法但无锁 — 外部直接调用读到不一致数据 | 中 | P1 |
| R4 | 双路径判断不统一 — `push_frame()` 用 `payload_kind` 枚举, `TranscodeStage` 用字符串 | 低 | P2 |
| R5 | `stop()` 设置 STOPPING 后不等待已执行的 `push_frame()` 完成 — flush 可能交叉 | 高 | P0 |
| R6 | 磁盘空间 TOCTOU — 预检时检查, 实际写入数小时后 | 中 | P2 |

### 2.3 infrastructure/ 层 (23 文件, ~3383 行)

**评分: 7.5/10**。子系统边界清晰, 无锁 SPSC ring 设计正确, 跨平台 SHM 抽象干净。

| # | 发现 | 风险 | 优先级 |
|---|------|------|--------|
| I1 | **FeishuWebhook::send() 空壳** — `return true`, 告警静默丢失 | **高** | P0 |
| I2 | **FFmpegEncoder 线程不安全** — 无 mutex, 多流并发导致 AVCodecContext 崩溃 | **高** | P0 |
| I3 | ConfigLoader 全局无锁 — 假定单线程, 但 UI+worker 并发访问风险 | 中 | P1 |
| I4 | PluginRegistryService::plugins_ 读写竞争 — `enablePlugin()` 无锁写入 vs `getSources()` 无锁读取 | 中 | P1 |
| I5 | PluginRingReader checksum 过弱 — XOR 前 256 字节, 大帧碰撞概率高 | 中 | P2 |
| I6 | StreamWriter 析构不写 trailer — 异常路径 MP4 不完整 | 中 | P2 |
| I7 | PluginRegistryService 锁嵌套风险 — stall_callback 在 registry_mutex_ 内触发, crash 处理又获取同一锁 | 中 | P1 |
| I8 | FPS 硬编码 = 30 — `ensure_context()` 忽略实际流帧率 | 低 | P3 |
| I9 | Win32 SHM unlink 是 no-op — 异常退出可能残留共享内存 | 低 | P3 |

### 2.4 micecam_ui/ 层 (9 文件, ~1800 行)

**评分: 4/10**。架构清晰但存在高风险数据竞争, 测试覆盖率为零。

| # | 发现 | 风险 | 优先级 |
|---|------|------|--------|
| U1 | **5 处数据竞争** — `total_frames_`/`bytes_written_`/`stream_frame_counts_`/`stream_drop_counts_`/`active_streams_` 在 capture 和 UI 线程间无同步 | **高** | P0 |
| U2 | **析构函数不停止 capture 线程** — 窗口关闭时 std::terminate | **高** | P0 |
| U3 | **编码器名/码率硬编码** — `current_encoder_name_ = "H.264"`, `current_bitrate_ = "5.0 Mbps"`, 非来自实际编码器 | 中 | P1 |
| U4 | refreshLiveStatus 信号风暴 — 30 次/秒 QML property 更新 + 日志条目重分配 | 中 | P1 |
| U5 | AppSettings 同步写盘 — 每个 setter 立即 `config_.save()`, 批量设置时多次 IO | 低 | P2 |
| U6 | **测试覆盖率 ≈ 0%** — 全 UI 层仅 MockCameraModel 有测试, 录制/崩溃/断开路径全未覆盖 | **高** | P0 |

---

## 三、Spec 完成度逐项对照

### Spec 001 (v2-rewrite)

| FR | 标题 | 状态 | 证据 |
|----|------|------|------|
| FR-001 | Plugin backend system | ✅ | PROJECT_ATLAS S10 |
| FR-002 | CameraStream abstraction | ✅ | PROJECT_ATLAS S10 |
| FR-003 | Transcode stage | ✅ | PROJECT_ATLAS S10 |
| FR-004 | Multi-stream MP4 output | ✅ | PROJECT_ATLAS S10 |
| FR-005 | SRT subtitle track | ✅ | PROJECT_ATLAS S10 |
| FR-006 | Session metadata JSON | ✅ | PROJECT_ATLAS S10 |
| FR-007 | Session statistics JSON | ✅ | PROJECT_ATLAS S10 |
| FR-008 | Timestamp system | ✅ | PROJECT_ATLAS S10 |
| FR-009 | Watchdog mechanism | ✅ | PROJECT_ATLAS S10 |
| FR-010 | Alert types | ⚠️ 部分 | 7 类型定义完整, 但触发逻辑分散 (`PROJECT_ATLAS S10:397`) |
| FR-011 | Preflight validation | ✅ | PROJECT_ATLAS S10 |
| FR-012 | spdlog async logging | ✅ | PROJECT_ATLAS S10 |
| FR-013 | Fault recovery | ⚠️ 部分 | finalize+reconnect 存在, corrupted frame skip 标记缺失 (`PROJECT_ATLAS S10:400`) |
| FR-014 | Hardware resource lock | ✅ | ResourceManager exclusive_resource_id 完整 |
| FR-015 | Cross-platform build | ⚠️ 部分 | macOS 确认, Win/Linux HIL 仅部分通过 (`PROJECT_ATLAS S10:402`) |

**spec 001 eval 明确 defer**: FeishuWebhook::send() 空壳 / 录制中磁盘满检测 / 完整 5 流 CI 测试

### Spec 002 (remove-ui-mock-data)

| FR | 标题 | 状态 | 证据 |
|----|------|------|------|
| FR-001 | CameraDetail 下拉框绑定模型 | ✅ | UI atlas S10 REQ-UI-001 |
| FR-002 | 硬编码 "00:42:17" 替换 | ✅ | `elapsedText()` 正常计时 |
| FR-003 | main.qml 信号处理绑定模型 | ✅ | UI atlas S10 REQ-UI-001 |
| FR-004 | OutputSettings 绑定 outputDirectory | ✅ | UI atlas S10 REQ-UI-010 |
| FR-005 | LoggingSettings 替换 mock 日志 | ✅ | `recentLogEntries` Q_PROPERTY 存在 |
| FR-006 | FullscreenCameraView 绑定模型 | ✅ | UI atlas S10 REQ-UI-001 |
| FR-007 | CameraDetail 空/零默认值 | ✅ | UI atlas S10 REQ-UI-006 |
| FR-008 | elapsedText 动态计时 | ✅ | UI atlas 伪代码 S12 含 HH:MM:SS 逻辑 |
| FR-009 | AppSettings 12 新属性 | ✅ | UI atlas S10 REQ-UI-010, ConfigLoader 18 字段 |
| FR-010 | recentLogEntries 暴露日志 | ✅ | UI atlas S10 REQ-UI-013 |
| FR-011 | Settings 开关绑定 | ✅ | UI atlas S10 REQ-UI-010 |

**残留**: 编码器名/码率硬编码 (见 spec 006 FR-005)

### Spec 003 (camera-plugin-runtime)

无独立 eval。FR 001-024 中: 共享内存传输 ✅、gRPC 控制 ✅、配置 schema ✅、设备分组 ✅。**缺口**: gRPC NotifyStallFn stub 未接入主进程 (spec 005 eval defer)。

### Spec 004 (production-ready)

| 项 | 状态 | 证据 |
|----|------|------|
| FR-001~025 实现 | ✅ 27/28 | spec 004 自身, atlas 交叉确认 |
| FR-026 dev→main 合并 | ❌ | spec 004 行 66, 被 flaky test 阻塞 |
| Flaky: StallCountResetsOnActivity | ❌ | spec 004 行 67; spec 007 Q14 提到修复但未验证 |
| HIL 测试文件 | ❌ | spec 004 行 71: `test_hil_e2e`/`test_hil_crash_recovery` 未创建 |

### Spec 005 (stream-monitoring-and-test-suite)

| 项 | 状态 | 证据 |
|----|------|------|
| FR-001~020 实现 | ✅ | spec 005 eval 39/39 测试通过 |
| HIL e2e | ❌ 硬件 defer | eval 行 126 |
| HIL crash recovery | ❌ 硬件 defer | eval 行 126 |
| gRPC stub 接入 | ❌ defer | eval 行 127 |
| FR-006 升级超时单测 | ❌ 设计级验证 | eval 行 128 |
| Windows CI fork e2e | ❌ defer | eval 行 130 |

### Spec 006 (cross-platform-compat)

| FR | 标题 | 状态 | 证据 |
|----|------|------|------|
| FR-001/002 | SharedMemoryBackend + 重构 | ✅ | infra atlas S6 | SHM 抽象层完整 |
| FR-003/004 | GetCapabilities 一致性 + 设备检查 | ⚠️ | OAK 返回 acknowledged=false |
| FR-005 | UI 编码器/码率绑定 | ❌ | UI atlas S8:648 硬编码残留 |
| FR-006 | elapsedText HH:MM:SS | ✅ | UI atlas 伪代码 S12 |
| FR-007 | preflightItems 接入后端 | ✅ | UI atlas S10 REQ-UI-004 |
| FR-008 | 去重 PayloadKind | ✅ | RecordingPipeline.h 使用 `using` |
| FR-009 | zero-init available_bytes_ | ✅ | 静态分析确认 |
| FR-010/011 | OpenStream 双开守卫 + Calibrate 验证 | ✅ | spec 006 审计 disposition 确认 |
| FR-012 | 平台感知 transport string | ✅ | `kShmTransportType` 常量 |
| FR-013 | old/ 目录移除 | ✅ | `.gitignore` 含 `old/` |
| FR-014 | API include 卫生 | ✅ | spec 006 审计 disposition 确认 |
| FR-015 | 缓冲区大小修正 | ✅ | spec 006 审计 disposition 确认 |
| FR-016 | Windows 信号处理 | ⚠️ | 需 Windows CI 验证 |
| FR-017 | 并发 stress test | ⚠️ | infra atlas S11 标"不足" |

### Spec 007 (plugin-ui-integration)

project_index 标 STATUS: COMPLETE。35 FR 已实现, 45/45 测试通过, 4/4 HIL 通过。

| 遗留 | 来源 |
|------|------|
| 打包验证 (macOS/Windows) | project_index |
| UI 截图匹配 visual anchors | project_index |
| OAK-D 硬件 waiver | project_index |
| Q14 修复 StallCountResetsOnActivity | spec 007 行 121-126 — 使用 cycle_count_ 同步 |

---

## 四、全局风险矩阵

| 等级 | 风险 | 模块 | 阻塞 |
|------|------|------|------|
| &#x1F534; 高 | FeishuWebhook::send() 空壳 | infra | 生产告警 |
| &#x1F534; 高 | AppController 5 处数据竞争 | UI | 数据一致性 |
| &#x1F534; 高 | AppController 析构不停止线程 | UI | 崩溃 |
| &#x1F534; 高 | RecordingPipeline stop/push 交叉 | pipeline | 数据损坏 |
| &#x1F534; 高 | start() 中途失败不清理 | pipeline | 资源泄漏 |
| &#x1F534; 高 | FFmpegEncoder 线程不安全 | infra | 多流崩溃 |
| &#x1F534; 高 | 测试覆盖率 ≈ 0% (UI 层) | UI | 质量无保障 |
| &#x1F7E1; 中 | encoding name / bitrate 硬编码 | UI | 不符合 spec 006 |
| &#x1F7E1; 中 | PluginRegistryService 锁嵌套 | infra | 潜在死锁 |
| &#x1F7E1; 中 | PluginRingReader checksum 弱 | infra | 数据完整性 |
| &#x1F7E1; 中 | StreamWriter 析构不写 trailer | infra | MP4 不完整 |
| &#x1F7E1; 中 | TimestampEngine 无初始化守卫 | domain | 无效时间戳 |
| &#x1F7E1; 中 | PluginRegistry 裸指针悬空 | domain | use-after-free |
| &#x1F7E1; 中 | Flaky test (macOS) | 全局 | 阻塞合并 |
| &#x1F7E1; 中 | HIL 测试未完成 | 全局 | 硬件门 |
| &#x1F7E1; 中 | gRPC NotifyStallFn stub | 全局 | 故障恢复 |
| &#x1F7E2; 低 | SessionMetadata 往返不对称 | domain | 持久化 |
| &#x1F7E2; 低 | discover_all 无 try-catch | domain | 设备发现 |
| &#x1F7E2; 低 | AppSettings 同步写盘 | UI | 性能 |
| &#x1F7E2; 低 | FPS 硬编码 30 | infra | 帧率不匹配 |
| &#x1F7E2; 低 | Win32 SHM unlink no-op | infra | 资源泄漏 |
| &#x1F7E2; 低 | 打包验证 | 全局 | 发布 |

---

## 五、合并阻塞项

当前 `dev` → `main` 合并链路的阻塞节点:

```
dev ──[阻塞: flaky test StallCountResetsOnActivity]──> main
        │
        ├── spec 004 C-1: 修复 macOS 定时竞态
        ├── spec 007 Q14: 声称用 cycle_count_ 替代 sleep_for 修复
        └── 状态: 未闭合 (spec 004 仍列出为 OPEN)
```

---

## 六、建议路线图

### 阶段 A — 合并前必须修复 (P0)

| # | 项 | 模块 | 工时 |
|---|------|------|------|
| A1 | 修复 flaky test `StallCountResetsOnActivity` | tests | 2h |
| A2 | AppController 析构加 `stopCaptureLoop()` | UI | 0.5h |
| A3 | `total_frames_`/`bytes_written_` 改 `atomic<uint64_t>` | UI | 2h |
| A4 | `RecordingPipeline::stop()` 加 drain 或 state 检查 | pipeline | 3h |
| A5 | `RecordingPipeline::start()` 失败回滚逻辑 | pipeline | 2h |

### 阶段 B — 生产就绪前 (P0)

| # | 项 | 模块 | 工时 |
|---|------|------|------|
| B1 | 实现 `FeishuWebhook::send()` HTTP POST | infra | 3h |
| B2 | `stream_frame_counts_` 加 mutex | UI | 2h |
| B3 | 修复 `current_encoder_name_`/`current_bitrate_` 硬编码 | UI | 2h |
| B4 | FFmpegEncoder 文档标注单线程约束 或 加锁 | infra | 1h |
| B5 | gRPC NotifyStallFn 接入主进程 | 全局 | 4h |

### 阶段 C — 质量加固 (P1)

| # | 项 | 模块 | 工时 |
|---|------|------|------|
| C1 | TimestampEngine 加 `bool anchored_` 守卫 | domain | 1h |
| C2 | PluginRegistryService 分离锁 | infra | 2h |
| C3 | PluginRingReader checksum 升级 CRC32 | infra | 1h |
| C4 | StreamWriter 析构 best-effort 写 trailer | infra | 1h |
| C5 | 补充 AppController 单元测试 | tests | 4h |
| C6 | HIL 测试 (e2e + crash recovery) | tests | 6h |
| C7 | macOS/Windows 打包验证 | CI | 4h |

### 阶段 D — 生产优化 (P2)

| # | 项 | 模块 | 工时 |
|---|------|------|------|
| D1 | `snapshot()` 加锁 或 改为 private | pipeline | 0.5h |
| D2 | PluginManifest::validate 加 id 格式校验 | domain | 0.5h |
| D3 | SessionMetadata 补充 FR-13 字段 | domain | 1h |
| D4 | discover_all 加 try-catch | domain | 0.5h |
| D5 | AppSettings debounce 批量写盘 | UI | 1h |
| D6 | 降低 refreshLiveStatus 频率 | UI | 0.5h |
| D7 | 预分配 capture 帧 buffer | UI | 1h |

---

## 七、证据索引

| 证据 | 文件 |
|------|------|
| Atlas domain S13 审计结论 (F1-F8) | `atlas/internal/domain/MODULE_ATLAS.md:635-666` |
| Atlas pipeline S13 审计结论 (R1-R6) | `atlas/internal/pipeline/MODULE_ATLAS.md:657-679` |
| Atlas infrastructure S13 审计结论 (I1-I9) | `atlas/internal/infrastructure/MODULE_ATLAS.md:785-814` |
| Atlas UI S13 审计结论 (U1-U6) | `atlas/cmd/micecam_ui/MODULE_ATLAS.md:946-1009` |
| Atlas 项目级 S9 风险矩阵 | `atlas/PROJECT_ATLAS.md:367-380` |
| Atlas 项目级 S10 需求符合性 | `atlas/PROJECT_ATLAS.md:384-404` |
| Spec 001 eval defer | `specs/001-micecam-v2-rewrite/eval.md:89-98` |
| Spec 004 开放项 | `specs/004-production-ready-plugin-app/spec.md:64-74` |
| Spec 005 eval defer | `specs/005-stream-monitoring-and-test-suite/eval.md:125-130` |
| Spec 006 FR-005 硬编码证据 | `atlas/cmd/micecam_ui/MODULE_ATLAS.md:648` |
| Spec 007 完成 + defer | `project_index:14` |
