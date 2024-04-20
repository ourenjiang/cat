set(meter_SRCS
meter/Meter.c
meter/CPUMeter.c

# 这5个功能是已经显示出来的
meter/CPUMeter.c         # CPU
meter/LoadAverageMeter.c # 平均负载
meter/MemoryMeter.c      # 内存
meter/SwapMeter.c        # 交换内存
meter/TasksMeter.c       # 任务
meter/UptimeMeter.c      # 持续运行时长

# 这3个功能并没有被显示出来，应该是在Linux中不支持.
meter/ClockMeter.c    # 时钟
meter/BatteryMeter.c  # 电池
meter/HostnameMeter.c # 主机名
)

set(panel_SRCS
panel/Panel.c
panel/MainPanel.c              # 主面板
panel/MetersPanel.c            # 功能面板
panel/AvailableMetersPanel.c   # 可用功能面板
panel/AvailableColumnsPanel.c  # 可用列面板
panel/ColumnsPanel.c           # 列面板
panel/CategoriesPanel.c        # 分类面板
panel/AffinityPanel.c          # 同类面板
panel/SignalsPanel.c           # 信号面板
panel/DisplayOptionsPanel.c    # 显示选项面板
panel/ColorsPanel.c            # 颜色面板
panel/ScreensPanel.c           # 屏幕面板
)

set(screen_SRCS
screen/ScreenManager.c     # 屏幕管理器
screen/OpenFilesScreen.c   # 打开文件屏幕
screen/TraceScreen.c       # 追踪屏幕
screen/InfoScreen.c        # 信息屏幕
screen/EnvScreen.c         # 环境屏幕
)

set(linux_SRCS
linux/PerfCounter.c
linux/Platform.c
linux/LinuxProcess.c
linux/LinuxProcessList.c
linux/LinuxCRT.c
linux/IOPriorityPanel.c
linux/Battery.c
)

set(base_SRCS
base/Hashtable.c
base/Vector.c
base/XAlloc.c
base/Object.c
base/CheckItem.c
base/ListItem.c
base/RichString.c
base/StringUtils.c
base/CRT.c
)

add_executable(htop
main.c
htop_other.c
Header.c
Process.c
UsersTable.c
Settings.c
Action.c
ProcessList.c
FunctionBar.c
IncSet.c
Affinity.c
${meter_SRCS}
${panel_SRCS}
${screen_SRCS}
${linux_SRCS}
${base_SRCS}
)

target_link_libraries(htop
ncursesw
m
)
