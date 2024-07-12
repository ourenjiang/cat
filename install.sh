#!/bin/sh

#注意，该脚本由安装包的部署者使用。
# 这里应由外部脚本将安装包解压，并进入解压目录;

# 1, 安装配置文件
cp\
    main/etc/paceic_ems_station.service\
    /lib/systemd/system/

# 2, 更新缓存
systemctl daemon-reload

# 3, 自启动使能
systemctl enable paceic_ems_station.service
