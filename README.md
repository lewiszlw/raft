# raft
Raft 协议的 C++ 实现。

Raft 协议
- 中文：https://github.com/maemual/raft-zh_cn/blob/master/raft-zh_cn.md
- 英文：doc/raft.pdf

## 开始


## 参考
Raft实现
- https://github.com/wenweihu86/raft-java
- https://github.com/logcabin/logcabin

C++ 代码规范：https://zh-google-styleguide.readthedocs.io/en/latest/google-cpp-styleguide/

## 问题记录
1.编译grpc时报错：c++: fatal error: Killed signal terminated program cc1plus
```
# 创建分区路径
sudo mkdir -p /var/cache/swap/
# 设置分区的大小
# bs=64M是块大小，count=64是块数量，所以swap空间大小是bs*count=4096MB=4GB
sudo dd if=/dev/zero of=/var/cache/swap/swap0 bs=64M count=64
# 设置该目录权限
sudo chmod 0600 /var/cache/swap/swap0
# 创建SWAP文件
sudo mkswap /var/cache/swap/swap0
# 激活SWAP文件
sudo swapon /var/cache/swap/swap0
# 查看SWAP信息是否正确
sudo swapon -s
————————————————
版权声明：本文为CSDN博主「tiffiny10」的原创文章，遵循CC 4.0 BY-SA版权协议，转载请附上原文出处链接及本声明。
原文链接：https://blog.csdn.net/weixin_44796670/article/details/121234446
```