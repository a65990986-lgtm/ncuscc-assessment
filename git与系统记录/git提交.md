mkdir -p ~/ncuscc-assessment/基础题
cd ~/ncuscc-assessment

cp "/home/kim/Desktop/New Folder/超算俱乐部的基础题/第一题.md" 基础题/
cp "/home/kim/Desktop/New Folder/超算俱乐部的基础题/第二题.md" 基础题/
cp "/home/kim/Desktop/New Folder/超算俱乐部的基础题/第三题.md" 基础题/

ls -l 基础题/

HTTPS污染严重,SSH不能浏览页面而且windows不能用,但是稳定.

试一试SSH通不通
ssh -T git@github.com

没问题,这是GitHub只让用它做 Git 操作
Hi a65990986-lgtm! You've successfully authenticated, but GitHub does not provide shell access.

然后这样做.实际连接的主机改为 ssh.github.com.用 443 端口连接,而不是默认的 22.22接口是SSH默认的端口,经常被经常被运营商限制/污染,连不上.443是HTTPS 默认端口GitHub 很贴心地提供了 ssh.github.com 这个入口，它支持在 443 端口上跑 SSH 协议。所以你把 SSH 流量伪装成走 443，就绕过了 22 端口被封的问题。
cat >> ~/.ssh/config <<'EOF'
Host github.com
  HostName ssh.github.com
  Port 443
  User git
EOF

进入目录
cd ~/ncuscc-assessment

在当前目录初始化一个仓库,指定当前分支是main.不然会是master这种现在比较少见的名称.
git init -b main

把"基础题/"这个文件加里面的所有东西都添加到暂存区
git add 基础题/

看看状态,哪些是已暂存/没跟踪/被修改
git status

提交.用-m来写提交信息.不用-m会进入nano来编辑提交信息,如果nano没有写任何东西,git会不让提交.不写提交信息几天之后谁知道这里面是什么东西呢?
git commit -m "基础题 1-3：找到入口 / 两段信号 / 读日志并修复输入"

查看提交历史，--oneline 让每条记录只显示一行(简短哈希 + 说明)
git log --oneline

git remote：管理远程仓库（remote）的命令,add：添加一个远程仓库,origin：给这个远程起个名字，习惯上默认就叫 origin（意思是"源头/老家"）
git remote add origin git@github.com:a65990986-lgtm/ncuscc-assessment.git

列出当前配置的所有远程仓库及其地址
git remote -v

会得到
origin	git@github.com:a65990986-lgtm/ncuscc-assessment.git (fetch)
origin	git@github.com:a65990986-lgtm/ncuscc-assessment.git (push)

git push：推送，把本地提交上传到远程,origin：推到哪个远程,main：推哪个分支,-u：--set-upstream，建立本地分支和远程分支的追踪关系
git push -u origin main

补充
git 在转义显示中文文件名（不是文件坏了）。想让它正常显示，可以 git config core.quotepath false
cp的时候路径有空格记得用""