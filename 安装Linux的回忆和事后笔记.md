太过细节而且问ai很容易跨过去的坑不记录.当时太绕而且没意义的地方改成可以做更好的形式.

+ 为什么选择了原生Linux?
为了更大限度发挥硬件的能力,有更完整的Linux体验

- 我有两块硬盘,一块是西部数据的(win11专用),一块是致态(Linux专用).此外我有一个小米u盘来作为启动盘

# 一开始我是这样规划的,但是我发现完全不用这样,因为win11的盘默认是NTFS可以直接挂在在Linux的树上.
分区	大小	文件系统	挂载点/用途
共享杯子	100GB	NTFS	Windows/Linux 临时交换
EFI 引导	1GB	FAT32	/boot/efi
Swap	40GB	swap	休眠支持（32GB 内存）
根目录	200GB	ext4	/
用户目录	剩余约 1.6TB	ext4	/home

```
但是如果要挂载NTFS到Linux需要取消win11的快速启动,因为快速启动选项开启的win11其实不是真的完全关机,乱搞可能会弄坏西部数据的硬盘.不过我在之前就试过解决win11卡壁纸不能点应用的问题的时候就关闭了快速启动,因此这里不必担心.
```

* 然后是系统选择.我没有选择比较新的 Ubuntu 26.04.1 LTS ,因为这可能太新了,一次技术跨度很大的更新，引入了很多新东西,也就意味着可能不太稳定,不太适合新手.

> 所以我选择了 ubuntu-24.04.1-desktop-amd64.iso  ,（带 .1 或 .2 的，这是修正版，修复了很多初版bug）

- 然后是使用连接去下载,尝试了清华大学镜像站(连接没了),阿里云镜像站(拒绝访问).随后又尝试了中科大（USTC）镜像站,南京大学（NJU）镜像站,网易（163）镜像站都没成功.

) 随后尝试使用BT种子.使用迅雷完成了下载.那官方提供的哈希值对比,发现无误,说明每件没有被篡改过.

然后就是备份u盘.用 Ventoy 制作启动盘.然后把前面的.iso文件拖进去.

插入u盘,重启的过程中不断点击F12,选择小米u盘.选择Boot in normal mode（正常模式启动）.

* 选择系统语言的时候我选择了English.有两个原因: 避开中文路径的魔法/给自己创造使用英语的环境.

* 选择不联网,不然可能会开始下载东西而卡死.

> 到了" How do you want to install Ubuntu ? "就开始有坑了
> 有三种安装选项:
    > nstall Ubuntu alongside Windows Boot Manager（与 Windows 启动管理器共存）.据说可能会压缩现有的Win11的分区,然后自己切一块空间出来,很容易出问题.别选
    > Erase disk and install Ubuntu（擦除磁盘并安装 Ubuntu）.清空选中的磁盘,然后自动分区.
    > Manual installation（手动安装）.

* 我当时选择了手动安装,然后出现了一个麻烦的问题.当创建第五个分区的时候总是显示System program problem detected（检测到系统程序问题）.

* 然后我尝试重启.随后遇到第二个麻烦的问题.新建一个分区之后,有很大概率会自动出现一个1.13GB 的FAT32分区.把它删掉让它合并到freeplace,下一次创建目录还是会出现.

- 只能最多创建4个分区的问题,经过排查,发现是因为致态的那个盘是MBR格式的,然后这个格式只能创建4个主分区.GPT格式没有这个限制.
- Ubuntu的安装界面不让我把 MBR 转 GPT ,又显示 System program problem detected.然后我回到win11用管理员身份运行cmd,然后改好格式.用命令行可以不用在GUI找半天

然后回到安装界面发现Linux的一个特性:硬盘的名字是会变的,原来是nvme0n1,可能变成nvme1n1

顺带一提,Linux支持NTFS,但是为了避免跟微软的授权费,所以不会在安装界面直接提供NTFS

- 现在可以创建5个盘了,但是那个1.13 GB，FAT32的幽灵分区就是去不掉,我打算不要了.可是 next 就是不让点,是灰色的.
- 反反复复重启试了很多次,就是有幽灵分区,而且也不给我next
- 生气了,然后直接选择Erase disk and install Ubuntu（擦除磁盘并安装 Ubuntu）.
- 都点完基本的信息之后有个,Please remove the installation medium, then press ENTER:
- 然后拔掉u盘,点击回车.防止电脑再次通过u盘启动

* 一个奇怪的现象是:手机从链接Linux的USB拔下来都黑屏了一下,然后叫我重新输入密码.这可能是Linux 系统把手机当成了一张“USB 网卡”。你把它拔掉，系统突然发现“我网卡没了”，会触发一次底层设备扫描.更可能的原因是,现在的 5070 Ti laptop用的是 Ubuntu 自带的 nouveau 备胎驱动。这种基础驱动对“硬件热插拔”等突发事件的反应非常生硬，一遇到插拔就会黑一下屏幕重新绘制画面

+ 然后更换到中国服务器,准备更新系统和驱动,不然使用默认的外国服务器太卡.
Software & Updates界面.找到 Download from（下载自），把它改成 中国的服务器（比如 mirrors.aliyun.com 或 mirrors.tuna.tsinghua.edu.cn）

等待系统更新的时候,顺便把中文输入法弄好,选择自带的pinyin就可以了

> 安装驱动的时候绝对不要选择recommended系统推荐的版本,因为那没有open后缀.很多开发者的经验是,50 系显卡（Blackwell 架构）在 Linux 下必须使用带 -open 后缀的开源内核模块，否则极容易在重启后直接黑屏死机.

完成后reboot重启

* 出现过一次这样的情况,电脑突然卡住，在扩选复制内容的时候，电脑突然无法控制鼠标.
* 可能是在拖拽选择（扩选）文本时卡死，极大概率是因为新的 NVIDIA 驱动和 Ubuntu 默认的图形显示服务器（Wayland）发生了冲突。这种高负载的图形渲染操作很容易让刚刚安装的 -open 驱动“瞬间宕机”，这可能是是新显卡在 Linux 系统里非常典型的“驱动磨合期”反应
* 切到命令行界面Ctrl + Alt + F3（如果没反应，依次试 F4、F5、F6）,如果屏幕黑了一下，出现黑底白字的 login: 提示符，说明内核没死！输入你的用户名 kim 回车，再输入密码（盲敲）回车,进去后，输入 sudo reboot 重启电脑。这种方法能大概率保证系统数据不损坏
* 切换到Xorg,这是更稳定的选择,不代表老就不好.
* 按 Ctrl + Alt + F3，进入黑底白字的 tty3 界面,输入用户名和密码
* sudo nano /etc/gdm3/custom.conf用这个配置编辑文件
* 在打开的文本里，找到这一行：#WaylandEnable=false , 把它前面的 # 号删掉，变成 WaylandEnable=false .意思是永久禁用 Wayland，只用 Xorg
* nano的退出方式,是按 Ctrl + O 然后按 Enter 保存
* reboot重启电脑确保生效


 # 设置默认启动为 Linux，用 GRUB 菜单切换
>进入 BIOS 设置（开机按 F2 或 Del，看机型）。
>找到“启动顺序”或“Boot Priority”，把 致态2T（Ubuntu） 调到第一位。
>保存退出。以后每次开机，默认从致态2T启动，会先进入 GRUB 菜单。
>如果 GRUB 检测到了 Windows，菜单里会有“Windows Boot Manager”选项，选择它就能进 Windows。如果 GRUB 没检测到 Windows，你需要手动添加（但一般 Ubuntu 安装时勾选“检测其他系统”就会自动加入）。

