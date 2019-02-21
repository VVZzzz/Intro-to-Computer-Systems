# Cache-lab
-----------------------------
## 实验目的
- PartA: 实现一个Cache模拟器
- partB: 
## 实验材料及准备
- 修改csim.c 并执行make得到csim。

## 实验过程
### PartA: Cache-Simulator
1. 此处的Cache模型为：

    一个set_line的地址结构为：
    | tags(-t) | sets(-s) | block(-b)|
    |-----------|--------|------|
    set_line的内容结构为：
    | last_used | valid | tag | block |
    | --------- | ----- | --- | ----- |
    其中last_used标志，上一次是多久使用的，其值越小代表越久未使用
    一个set为：
    |set|
    |----|
    |line_0|
    |line_1|
    |...|
    |line_E-1|
    而一个Cache模型为：
    |Cache|
    |-----|
    |set_0|
    |set_1|
    |...|
    |set_2^s-1|

2. 参数为：-t 为tag位数，-s为sets位数，共2^s个set,-b 为块大小。-E 为一个set中共有E行。
3. mem_address为64位，即tag+s+b=64. 
4. 执行LRU(最近未使用)算法进行页替换
    - 首先Cache初始为空，那么此时要将read的块存入Cache.对于这种情况，我们首先要找空行，即find_emptyline.
    如果该行中的valid无效，代表为空，可以存入，返回其index。否则返回-1.
    - 
    


