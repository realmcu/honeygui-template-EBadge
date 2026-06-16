# HoneyGUI 项目 — AI 协作指南

本项目使用 HoneyGUI HML（XML-based UI 标记）描述嵌入式 GUI。
目标引擎（targetEngine）：**honeygui**（见 project.json，每个项目锁定一个引擎）。

## 规范（唯一真相源）

生成 / 修改 HML 前**必读**：[./HML-Spec.md](./HML-Spec.md)

- 仅使用规范中标注当前引擎 honeygui 为 ready(✓) 的组件；标注 planned / unsupported 的一律勿用。
- `hg_view` 不可嵌套；非容器组件不可有子组件。
- 资源路径必须以 `/` 开头（从 assets 目录起算）；`hg_label` 必须有 `fontFile`，且字体须在 assets/ 中。
- 事件用 `<events><event><action>` 结构，不用内联 `onXxx` 属性。
- 尺寸用 `width`/`height`（不是 `w`/`h`）；对齐用 `hAlign`/`vAlign`（不是 `textAlign`）。
- 绝不存在、永远勿用的组件：`hg_container`、`hg_grid`、`hg_tab`。

## 生成后必做（验证闭环）

调用 Extension HTTP API 验证（设计器运行时监听 38912）：

```bash
curl -X POST http://localhost:38912/api/validate-hml \
  -H "Content-Type: application/json" \
  -d '{"filePath":"ui/xxx.hml"}'
```

修复所有 errors 后再继续。注意：验证器只查 8 条结构规则，**不校验组件白名单 / 属性名**，
`valid:true` 仅必要不充分——仍须对照 HML-Spec.md 人工核对组件在当前引擎可用、属性名正确。

## 看效果（仿真）

在 VSCode 中打开本项目（已安装 HoneyGUI Visual Designer 扩展）：
点击 HoneyGUI 侧边栏的 **Simulate（🚀）**，或命令面板执行 `HoneyGUI: Simulate`，
扩展会自动完成 codegen → SCons 编译 → 仿真器运行。
