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

typedef struct CommandLineSettings_ {
   Hashtable* pidWhiteList;
   uid_t userId;
   int sortKey;
   int delay;
   bool useColors;
   bool treeView;
} CommandLineSettings;

CommandLineSettings parseArguments(int argc, char** argv);
void millisleep(unsigned long millisec);
void init_locale();// 初始化字符集
void clear_resource(Settings*, Header*, ProcessList*, ScreenManager*, UsersTable*, CommandLineSettings);// 清理资源
