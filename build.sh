#!/bin/sh

# 注意，该脚本由安装包的发布者使用。

# 编译
cmake -B ../server_build ./
cd ../server_build;make && make install
cd ../

# 创建根目录
mkdir -p paceic_ems_server/main/
# 创建数据库目录
mkdir paceic_ems_server/main/db/
# 创日志文件目录
mkdir paceic_ems_server/main/log/
# 将安装脚本拷贝至项目主目录
cp server/install.sh paceic_ems_server/
# 将cmake安装内容拷贝至主服务目录
cp -r server_build/install/* paceic_ems_server/main/
# 将项目主目录打包(带上当前时间戮)
tar zcf paceic_ems_server_`date +%Y%m%d%H%M%S`.tar.gz paceic_ems_server/
# 清理临时文件
rm -rf build paceic_ems_server
# 完成
echo "build success."
