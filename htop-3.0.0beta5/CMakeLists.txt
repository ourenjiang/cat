set(meter_SRCS
Meter.c

# 这5个功能是已经显示出来的
CPUMeter.c         # CPU          
LoadAverageMeter.c # 平均负载
MemoryMeter.c      # 内存
SwapMeter.c        # 交换内存
TasksMeter.c       # 任务
UptimeMeter.c      # 持续运行时长

# 这3个功能并没有被显示出来，应该是在Linux中不支持.
ClockMeter.c    # 时钟
BatteryMeter.c  # 电池
HostnameMeter.c # 主机名
)

set(panel_SRCS
Panel.c
MainPanel.c              # 主面板
MetersPanel.c            # 功能面板
AvailableMetersPanel.c   # 可用功能面板
AvailableColumnsPanel.c  # 可用列面板
ColumnsPanel.c           # 列面板
CategoriesPanel.c        # 分类面板
AffinityPanel.c          # 同类面板
SignalsPanel.c           # 信号面板
DisplayOptionsPanel.c    # 显示选项面板
ColorsPanel.c            # 颜色面板
ScreensPanel.c           # 屏幕面板
)

set(screen_SRCS
ScreenManager.c     # 屏幕管理器
OpenFilesScreen.c   # 打开文件屏幕
TraceScreen.c       # 追踪屏幕
InfoScreen.c        # 信息屏幕
EnvScreen.c         # 环境屏幕
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

add_executable(htop
htop.c
htop_other.c
Header.c
Process.c
UsersTable.c
Settings.c
CRT.c
ListItem.c
Action.c
XAlloc.c
Hashtable.c
ProcessList.c
StringUtils.c
RichString.c
Object.c
FunctionBar.c
IncSet.c
Affinity.c
Vector.c
Hashtable.c
CheckItem.c
${meter_SRCS}
${panel_SRCS}
${screen_SRCS}
${linux_SRCS}
)

target_link_libraries(htop
ncursesw
m
)
