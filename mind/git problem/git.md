# Git 常见操作及问题整理

## 常用操作

### 本地基础操作

```bash
git init        	# 初始化文件夹为git库
git add .       	# 添加文件至缓冲区
git commit -m "..."   	# 将缓冲区文件提交至归档区，同时对操作命名
```

```bash
revert [name] 		# 删除该次提交
```

```bash
git reset --mixed id  	# 将归档区版本调回工作区
git reset --soft id 	# 将归档区版本调回缓冲区
git reset --hard id 	# 删除上次提交后的全部更改
```

```bash
git status  		# 查看文件状态  绿色（缓存区状态） 红色（工作区状态）
git log 		# 查看提交历史  按 q 退出
git reflog 		# 查看操作记录，包含 rest 回退后记录
```

### 远程连接

```bash
git remote add origin [...].git 	# 连接远端库，多次操作无需重复
git remote rm origin  			# 重置连接
```

### 提交

```bash
git push origin master  			# 提交至远端库主分支
git push -u origin master 			# 强制提交
git push -f origin master 			# 忽略差异覆盖
```

```bash
git config --global credential.helper store 	# 记住密码，默认15 min
```

### 下拉

```bash
git pull origin master
```

```bash
git fetch origin [...]:[...]
git merge ...
```

### 分支操作

```bash
git branch -v			# 查看分支
git checkout [...] 		# 转到该分支
git checkout -b [...] 		# 添加本地分支并转到该分支
```

### 分支合并

```bash
git merge [...] 		# 将该分支合并至当前分支
git merge -s recursive -X theirs --allow-unrelated-histories  origin/Gridea
```

### .gitignore 忽略已经 push 文件

```bash
git rm -r --cached (file/dir name)
```

&nbsp;

## 问题整理

### 问题信息

```
! [remote rejected] master -> master (failure) error: failed to push some refs to 'https://github.com/lovelydayss/Leetcode.git'
```

### 解决方法

查找许多方法未能很好解决，最终选择暴力提交

```bash
git push -f origin master
```
