TESMIOAGRI — TESMIO / Republic Mod Loader 安装说明
====================================================

运行包结构
README.txt、previewimage.png 放在根目录；插件 DLL、INI 和 RML 元数据放在 plugins 目录。
本包面向 WRSR 1.1.1.9（64 位）。DLL 使用 TESMIO API 4；RML 使用其 TESMIO 兼容宿主加载。

TESMIO 安装
1. 完全退出游戏，从本包 plugins 目录复制 TESMIOAGRI.dll 和 TESMIOAGRI.ini 到：
   <游戏目录>\tesmioloader\build\plugins\（tesmioloader.dll 旁的 plugins 目录）。
2. 启用 TESMIO 插件扫描；若 [plugins] 显式关闭了 TESMIOAGRI，将其设为 1。
3. 使用 tesmiolauncher.exe 启动。RML 的 .rml.json 元数据无需复制给 TESMIO。

Republic Mod Loader（RML）安装
1. 订阅并下载对应创意工坊项目后，使用支持 TESMIO API 4 的 RML 启动器。
2. 刷新列表，在 Plugins 中启用 TESMIOAGRI；工坊文件应位于 <项目编号>\plugins\。
3. 手动安装时，将本包 plugins 内的 DLL、INI、.rml.json 复制到 RML 的 rml\plugins\。
4. INI 与实际加载的 DLL 放在同一目录。若已有手动安装副本，先备份个人设置再统一安装来源。

配置与发布
编辑实际加载目录内的 INI；保存为 UTF-8 无 BOM，并完整重启游戏。
同一个游戏进程选择一个 Loader 启动。可选的 Reputation 联动不构成强制依赖。
上传目录应包含 plugins、README.txt、previewimage.png 及该项目原有的 workshopconfig.ini。
保留各项目的 ITEM_ID、OWNER_ID 和可见性，使用 SCRIPT 项目类型；不要上传传输 ZIP。
本次调整验证了打包结构和接口；调整参数后的游戏效果及存档兼容性仍需实际运行验证。

English installation
====================
Target: WRSR 1.1.1.9 x64; TESMIO API 4 or RML with the TESMIO API 4 compatibility host.
TESMIO: copy TESMIOAGRI.dll and TESMIOAGRI.ini from this package's plugins folder into
<game>\tesmioloader\build\plugins, enable the plugin, and launch with tesmiolauncher.exe.
RML Workshop: subscribe/download the item, refresh the inventory and enable TESMIOAGRI.
RML manual: copy DLL, INI and .rml.json from plugins into RML's rml\plugins folder.
Keep the INI beside the DLL that is actually loaded. Edit settings, then fully restart.
Use one loader per game process. Reputation integration is optional, not a required dependency.
Publish the plugins folder with root README/preview and the item's original workshopconfig.ini;
preserve item/owner IDs and visibility, use SCRIPT type, and do not upload the transfer ZIP.
Package/interface checks do not establish gameplay behavior or save compatibility.

功能与参数 / Features and settings
====================================================
TESMIOAGRI 0.1.0

中文
====================

Workers & Resources: Soviet Republic 的 TESMIO 农业插件，可独立调整作物成熟时间与收获产量
安装
1. 退出游戏，将 TESMIOAGRI.dll 和 TESMIOAGRI.ini 放入 tesmioloader.dll 旁的 plugins 目录。
2. 在 TESMIO 启动器中启用插件，修改安装目录中的 INI 后完整重启游戏。
配置
[agriculture]
enabled = 1
maturity_time_scale = 1.0
yield_multiplier = 1.0
参数: 作用
enabled: 1 启用，0 关闭
maturity_time_scale: 正向生长时间倍率：0.5 约半时长，2 约两倍时长
yield_multiplier: 新增收获数量倍率：0.5 一半，2 两倍

两个倍率范围均为 0.1–10.0，默认 1 保持原版。使用小数点，INI 保存为 UTF-8 无 BOM。

成熟时间受气候影响，不是固定天数；低温衰退不加速，播种、收割和季节约束仍保留。产量倍率不放大已有库存，也不扩大仓储容量。

检查与关闭

在新一次启动的 tesmioloader.log 中查看 TESMIOAGRI：identity settings 表示保持原版，installed 表示补丁已安装，refused 表示拒绝启用。

将 enabled=0 或在启动器取消勾选后重启，即可关闭。建议先用存档副本测试。

文件

- TESMIOAGRI.dll：农业插件。
- TESMIOAGRI.ini：成熟时间与收获产量配置。
- README.txt：中英文使用说明。
- previewimage.png：工坊预览图。

English
====================


A TESMIO agriculture plugin for Workers & Resources: Soviet Republic. Adjust crop maturity time and harvest yield independently, with support for manual and mechanical harvesting.
Installation

1. Exit the game. Copy TESMIOAGRI.dll and TESMIOAGRI.ini into the plugins folder next to tesmioloader.dll.
2. Enable the plugin in the TESMIO launcher. Edit the installed INI and fully restart the game after changing settings.

Configuration

[agriculture]
enabled = 1
maturity_time_scale = 1.0
yield_multiplier = 1.0

enabled: 1 enables the plugin; 0 disables it
maturity_time_scale: Positive-growth duration multiplier: 0.5 is roughly half the wait; 2 is roughly twice the wait
yield_multiplier: Newly harvested crop quantity multiplier: 0.5 halves it; 2 doubles it

Both multipliers accept 0.1–10.0. The default 1 preserves vanilla behavior. Use a decimal point and save the INI as UTF-8 without BOM.

Maturity time depends on climate and is not a fixed number of days. Cold-weather decay is not accelerated; sowing, harvesting and seasonal restrictions still apply. Yield settings do not multiply existing inventory or increase storage capacity.

Status and disabling

Check TESMIOAGRI entries in a fresh tesmioloader.log: identity settings means vanilla behavior, installed means patches were installed, and refused means activation was rejected.

Set enabled=0 or uncheck the plugin in the launcher, then restart. Test on a copy of your save first.

Files

- TESMIOAGRI.dll: agriculture plugin.
- TESMIOAGRI.ini: maturity-time and harvest-yield settings.
- README.txt: Chinese and English instructions.
- previewimage.png: Workshop preview image.

GitHub: https://github.com/HONGxuanzhao/TESMIOAGRI
