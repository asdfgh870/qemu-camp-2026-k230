# spi flash 测试
参考：https://www.kendryte.com/k230/zh/main/03_other/K230_SDK_FAQ_C.html
spi nor 和 spi nand flash识别
问题： 怎么知道evb板上连接的是spi nor还是spi nand flash？

答：方法1：子板丝印不一样，丝印会有nor或nand标识。

​ 方法2：linux启动log会有打印，比如连接spi nor时会有类似下面打印

[root@canaan ~ ]#dmesg | grep spi
[    1.299989] spi spi0.0: setup mode 0, 8 bits/w, 100000000 Hz max --> 0
[    1.306704] spi-nor spi0.0: gd25lx256e (32768 Kbytes)
[    1.311786] 2 fixed-partitions partitions found on MTD device spi0.0
[    1.318147] Creating 2 MTD partitions on "spi0.0":


# 启动一个k230 的linux虚拟机

参考：
K230 linux sdk 镜像下载：
https://download.kendryte.com/k230/release/linux_sdk_images/daily_build/
sdk构建说明：
https://www.kendryte.com/k230_linux/zh/main/userguide/sdk_description.html
K230 Linux SDK 新板子适配指南：
https://www.kendryte.com/k230_linux/zh/main/advanced_adaptation_guide/new_board_adaptation_doc.html
通用驱动开发指南 SPI 使用说明:
https://www.kendryte.com/k230_linux/zh/main/app_develop_guide/driver/spi.html

