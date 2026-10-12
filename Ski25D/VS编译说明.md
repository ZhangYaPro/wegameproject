# Visual Studio 编译修复记录

后续更新：陡坡挑战版已在编辑器关闭后，通过 `Ski25DEditor Win64 Development -WaitMutex -FromMsBuild -architecture=x64` 的标准构建，退出码为 0。当前模块清单指向标准 `UnrealEditor-Ski25D.dll`，不再依赖之前的 `-2027` 临时模块。下文保留此前问题的排查记录。

## 已确认的原因

2026-10-10 按错误列表中的原命令执行 `Ski25DEditor Win64 Development -WaitMutex -FromMsBuild -architecture=x64`，UnrealBuildTool 报告：

```text
Unable to build while Live Coding is active. Exit the editor and game,
or press Ctrl+Alt+F11 if iterating on code in the editor or game
```

这是真实构建失败的原因。当前编辑器进程仍启用了 Live Coding，修改磁盘配置不会关闭已经启动的会话。

另一个问题是 VS 工程过期：原工程没有收录新增的三个类（六个 `.h/.cpp` 文件），也没有 ProceduralMeshComponent 头文件搜索路径。单独打开这些未收录的文件，代码分析无法得到正确的 Unreal 平台宏、生成头和预编译头环境，会产生连锁的类型和宏诊断。

## 已完成

- 用 UnrealBuildTool 重新生成 `Ski25D.sln`、工程文件和 IntelliSense 数据。
- 核对新工程已收录全部六个新文件和 ProceduralMeshComponent 的头文件路径。
- 在 `Config/DefaultEditorPerProjectUserSettings.ini` 将项目默认 Live Coding 关闭，并设置为手动启动。
- 用 MSVC 14.38 的 x64 编译器，按 Unreal 生成的响应文件，对三个新 `.cpp` 执行不依赖 PCH 的 `/Zs /Y-` 语法检查：全部退出码 0。
- 独立后缀模块 `UnrealEditor-Ski25D-2027.dll` 链接成功，保留当前样板可运行的模块输出。

没有修改 Unreal 引擎源码，也没有通过关闭错误诊断来隐藏问题。

## 在当前打开的窗口中完成

1. 保存关卡，关闭 Unreal 编辑器及 Live Coding 窗口。不要在 Live Coding 活跃时执行 VS 的完整构建。
2. 保存 VS 中未保存的修改，然后关闭并重新打开项目根目录的 `Ski25D.sln`；如果 VS 提示工程文件已变化，选择重新加载。
3. 选择 **Development Editor / Win64**，生成 **Ski25D** 项目。
4. 等待 IntelliSense 重新分析，再查看错误列表。原来的旧诊断不能作为修复后构建结果。

项目内的配置仅改变后续启动的默认值。若你此前在编辑器偏好设置里单独开启过 Live Coding，请同时取消“Enable Live Coding”。需要热更新时，可以主动重新启用，并使用 Ctrl+Alt+F11 的热更新工作流。

截至本次修复结束，尚未确认正在打开的 VS 错误列表已清空，编辑器关闭后的普通 VS 完整构建也需要再验证。原工程和配置备份位于 `Backup/BeforeVSRepair`。
