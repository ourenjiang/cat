/*
htop - htop.c
(C) 2004-2011 Hisham H. Muhammad
Released under the GNU GPL, see the COPYING file
in the source distribution for its full text.
*/

#include "config.h"

#include "FunctionBar.h"
#include "Hashtable.h"
#include "ColumnsPanel.h"
#include "CRT.h"
#include "MainPanel.h"
#include "ProcessList.h"
#include "ScreenManager.h"
#include "Settings.h"
#include "UsersTable.h"
#include "Platform.h"

#include <getopt.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "htop_other.h" // 将辅助函数拆分出去


/**
 * 1, 设置字符集
 * 2, 解析命令行参数
 * 3, 
 * 
*/

int main(int argc, char** argv)
{
   init_locale();// 设置字符集
   CommandLineSettings flags = parseArguments(argc, argv); // 获取命令行参数

   /**
    * 设置列宽 ??
    * 不执行，好像也没什么问题。
   */
   // Process_setupColumnWidths();

   /**
    * 用户表 和 进程列表，两者的关系是怎样的？
    * UsersTable 保存了系统中所有用户的信息
    * ProcessList 保存了所有进程的信息
   */
   UsersTable* ut = UsersTable_new();
   // 为进程列表 => 关联上用户属性，同时设置进程白名单和所关注的用户ID
   ProcessList* pl = ProcessList_new(ut, flags.pidWhiteList, flags.userId);

   // 初始化全局设置
   Settings* settings = Settings_new(pl->cpuCount);
   pl->settings = settings;// 给ProcessList添加设置信息

   /**
    * 注意，这里也给Header添加了设置信息
    * 
   */
   Header* header = Header_new(pl, settings, 2);
   Header_populateFromSettings(header);// 从设置参数中生成头部内容

   // 字符界面配置初始化
   {
      if (flags.delay != -1)
         settings->delay = flags.delay;
      if (!flags.useColors) 
         settings->colorScheme = COLORSCHEME_MONOCHROME;
      if (flags.treeView)
         settings->screens[0]->treeView = true;

      CRT_init(settings->delay, settings->colorScheme);
   }
   
   // 主面板初始化
   MainPanel* panel = MainPanel_new();
   ProcessList_setPanel(pl, (Panel*) panel);// 进程列表->设置面板

   MainPanel_updateTreeFunctions(panel, settings->screens[0]->treeView);// 主面板->更新树型功能

   if (flags.sortKey > 0) {
      settings->screens[0]->sortKey = flags.sortKey;
      settings->screens[0]->treeView = false;
      settings->screens[0]->direction = 1;
   }
   ProcessList_printHeader(pl, Panel_getHeader((Panel*)panel));// 进程列表->打印头部

   // 为主面板设置状态
   State state = {
      .settings = settings,
      .ut = ut,
      .pl = pl,
      .panel = (Panel*) panel,
      .header = header,
   };
   MainPanel_setState(panel, &state);
   
   // 初始化屏幕管理器
   ScreenManager* scr = ScreenManager_new(0, header->height, 0, -1, HORIZONTAL, header, settings, true);
   ScreenManager_add(scr, (Panel*) panel, -1);// 向屏幕管理器 添加 一个面板

   // 这部分还没看出来干嘛的
   // ProcessList_scan(pl);
   // millisleep(75);
   // ProcessList_scan(pl);

   ScreenManager_run(scr, NULL, NULL, NULL);
   
   clear_resource(settings, header, pl, scr, ut, flags);// 资源清理
   return 0;
}
