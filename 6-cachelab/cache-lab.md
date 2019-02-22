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



### PartB: 使用blocking技术完成矩阵的转置
根据材料得知,s=5 b=5,共有32个sets,一个sets的块大小为32Byte.即一个块可以放入8个int.
故如果按照默认的来排列(假设为32*32),则情况如下.
  ![](https://i.imgur.com/t2WrTbG.png) 
即按顺序存入cache时,第一行和第九行会发生conflict miss.这样会发生一个问题,转置为```B[j][i]=A[i][j]```.在对矩阵B的时候,这样默认的方式会造成更多的miss.所以要采用blocking技术.

- M == 32 N == 32
将32*32的矩阵分成8*8的小block,这样会减少miss数目.如下图分析:
![](https://i.imgur.com/CP54U2s.png)
其中1-8 9-16....等都会各自存入一个set中,这样在转置到B的时候,B的一行都可以从这8个set中取出,即hit.比默认的miss数目大量减少.
故我们得到结果如下:
![](https://i.imgur.com/sUehfDJ.png) 
我们发现miss数目仍然大于300. 为进一步减小miss,我们发现对于对角线上的数目可以不用```B[j][i]=A[i][j]```.这样也就减少了miss数,对于对角线上的元素,用一个临时变量保存即可.再次实验:  
![](https://i.imgur.com/qwWT4hb.png)  

- M == 64 N ==64
我们继续按照8*8的块大小来分,发现次数大于要求的miss.
