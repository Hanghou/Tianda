# 项目文档

## 文档结构

### 项目文档
- [快速重构指南](QUICK_RESTRUCTURE_GUIDE.md) - 最小改动的重构方案
- [项目重构计划](PROJECT_RESTRUCTURE_PLAN.md) - 完整的重构计划
- [技能文档](SKILL.md) - 开发技能和经验总结

### 使用指南
- [振镜启用指南](guides/ENABLE_GALVO_GUIDE.md)
- [振镜功能总结](guides/GALVO_ENABLED_SUMMARY.md)
- [光谱仪完成总结](guides/SPECTROMETER_COMPLETION_SUMMARY.md)
- [Git 推送指南](guides/GIT_PUSH_GUIDE.txt)

## 模块文档

每个模块都有独立的 README.md 文件：

- [激光器模块](../LaserDriver/README.md)
- [光谱仪模块](../Spectrometer/README.md)
- [振镜模块](../GalvoMirror/README.md)
- [位移台模块](../StageController/README.md)
- [延迟线模块](../DelayLine/README.md)
- [通信基类](../Communication/README.md)
- [工具类](../Utils/README.md)

## 文档规范

### 模块 README 结构
每个模块的 README.md 应包含：
1. 功能概述
2. 通信协议/硬件要求
3. 主要功能列表
4. 文件说明
5. 使用示例
6. 参考文档

### 代码注释规范
- 使用 Doxygen 风格注释
- 类和函数都要有说明
- 重要算法要有详细注释

### 提交信息规范
```
<type>: <subject>

<body>

<footer>
```

类型（type）：
- feat: 新功能
- fix: 修复 bug
- docs: 文档更新
- style: 代码格式调整
- refactor: 重构
- test: 测试相关
- chore: 构建/工具相关

## 更新记录

### 2024-02-03
- ✅ 创建 docs/ 目录结构
- ✅ 移动所有文档文件到 docs/
- ✅ 为所有模块创建 README.md
- ✅ 创建主 README.md
- ✅ 创建 .gitignore 文件
