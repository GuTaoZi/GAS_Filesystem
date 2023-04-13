<img src="https://s2.loli.net/2023/04/13/LnKtR6Qv2Yz4deh.png" alt="icon.png" width='500px' />

A custom Linux file system designed and implemented as a kernel module. Project for CS334 Operating System. [Task description here](https://github.com/oscomp/proj209-custom-filesystem).

## Contributors

| SID      | Name                                            | Contributions | Contribution Rate |
| -------- | ----------------------------------------------- | ------------- | ----------------- |
| 12111624 | [GuTaoZi](https://github.com/GuTaoZi)           |               |                   |
| 12110524 | [Artanisax](https://github.com/Artanisax)       |               |                   |
| 12112012 | [ShadowStorm](https://github.com/Jayfeather233) |               |                   |

## TODOs

The TODOs itself is now the main todo to do (^^ ;

## Requirements

### Project209-Linux custom filesystem

自己设计一个Linux 文件系统并实现文件和目录读写操作。

1、设计实现一个Linux内核模块，此模块完成如下功能：

  - 将新创建的文件系统的操作接口和VFS对接。
  - 实现新的文件系统的超级块、dentry、inode的读写操作。
  - 实现新的文件系统的权限属性，不同的用户不同的操作属性。
  - 实现和用户态程序的对接，用户程序

2、设计实现一个用户态应用程序，可以将一个块设备（可以用文件模拟）格式化成自己设计的文件系统的格式。

3、设计一个用户态的测试用例应用程序，测试验证自己的文件系统的open/read/write/ls/cd 等通常文件系统的访问。

## Changelog

See [CHANGELOG.md](https://github.com/GuTaoZi/GAS_Filesystem/blob/main/CHANGELOG.md).

