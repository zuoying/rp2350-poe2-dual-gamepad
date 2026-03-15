# RP2350-POE2-Dual-Gamepad
微雪RP2350-USB-A板实现单手柄输入→双虚拟手柄输出，带反作弊随机化，适配POE2双开。

## 硬件适配
- 微雪RP2350-USB-A开发板
- 有线Xbox游戏手柄（插USB-A口）
- Type-C线连接开发板与电脑

## 核心功能
1. 单手柄输入，Windows识别2个独立虚拟手柄；
2. View+Menu组合键切换3种模式：
   - 绿灯：同步模式（反作弊随机化）
   - 蓝灯：仅主角色
   - 红灯：仅副角色
3. 反作弊随机化：摇杆/扳机/按键微小偏移/延迟，模拟双真人操作。

## 使用步骤
1. 编译：
   - 克隆仓库：`git clone https://github.com/你的用户名/rp2350-poe2-dual-gamepad.git`
   - 本地编译：安装Pico SDK + ARM GCC，执行`mkdir build && cd build && cmake .. && make`；
   - 或直接下载GitHub Actions编译的UF2文件。
2. 烧录：
   - 按住开发板BOOTSEL键，Type-C接电脑（识别为U盘）；
   - 拖入`poe2_dual_gamepad.uf2`文件，开发板自动重启。
3. 验证：
   - 手柄插USB-A口，指示灯亮起；
   - Windows按Win+R输入`joy.cpl`，查看2个虚拟手柄；
   - 按View+Menu切换模式，RGB灯对应变色。

## 注意事项
- USB-A口5V供电由GPIO18控制，代码已默认开启；
- PIO USB基于GPIO12/13，系统时钟120MHz，不可修改；
- 反作弊随机化强度默认30，可在`main.c`中修改`g_rand_strength`。
