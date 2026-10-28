# 实验 2 简单文本编辑器

郭毅川 2024414290411 24软件工程4班

## 构建运行

使用 Qt Creator 打开 `src/editor.pro`。界面文件可在 Designer 中编辑。

```sh
mkdir -p src/build
cd src/build
qmake ../editor.pro
make -j2
./editor
```

实际验证环境：Linux、Qt 5.15.3、C++17。
测试在 `src/tests`，日志和截图在 `evidence`。

## 实验材料

专业：软件工程。完整报告为本目录的 `实验2报告_郭毅川_2024414290411.docx`。
运行截图与原始日志在 `evidence`，连续演示视频时长 2 分 10 秒。
最终 Qt Test 记录 16 passed、0 failed（包含初始化及清理）。

## 自动测试

```sh
mkdir -p src/tests/build
cd src/tests/build
qmake ../tests.pro
make -j2
QT_QPA_PLATFORM=offscreen ./test_editor
```

## 录制连续演示

`src/demo/demo.pro` 为自动演示驱动，使用真实 Qt 窗口与 Qt Test 输入。
安装 FFmpeg 后在 demo 的构建目录运行：

```sh
QT_QPA_PLATFORM=offscreen ./demo /绝对路径/演示.mp4 /绝对路径/ffmpeg /绝对路径/本实验仓库
```

演示保留真实运行时间和字幕，文件读写使用与菜单共用的接口。

UTF-8 纯文本仅保存文字；字体与颜色是视图设置。行号已实现，选做自动保存未启用。
