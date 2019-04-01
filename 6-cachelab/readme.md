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
4. 执行LRU(最近未使用)算法进行替换    
    - 首先Cache初始为空，那么此时要将read的块存入Cache.对于这种情况，我们首先要找空行，即```find_emptyline()```.
    如果该行中的valid无效，代表为空，可以存入，返回其index。否则返回-1.  
	此时找到空行,存入,并设置为该行的last_used值为所有最大值+1.表示刚使用过,不能替换他.
	- 之后如果都满了,就要进行替换(驱逐)了.对于这种情况,执行```find_evic_line()```,即找该```set```中最久未被使用的一行,也就是```last_used```值最小的一行.
5. 其余的即记录```hit```和```miss```即可.



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
元素排列情况如下:
![](https://i.imgur.com/31RlK1X.png)  
即我们发现第5行和第1行会conflict miss.
并且我们继续按照8*8的块大小来分,发现次数大于要求的miss.
所以我们采用以下措施:

	    for (row = 0; row < N; row += 8) {
	      for (col = 0; col < M; col += 8) {
	        for (r = row; r < row + 4; r++) {
	
			  //step 1
	          for (c = 0; c < 8; c++) buffer[c] = A[r][col + c]; 
	
			  //step 2
	          for (c = 0; c < 4; c++) {
	            B[col + c][r] = buffer[c];
	            B[col + c][r + 4] = buffer[c + 4];
	          }
	        }
	
	        for (c = col; c < col + 4; c++) {
		      //step 3 
	          for (r = 0; r < 4; r++) buffer[r] = B[c][row + 4 + r];
			  //step 4
	          for (r = 4; r < 8; r++) B[c][row + r] = A[row + r][c];
	
			  //step 5
	          for (r = 0; r < 4; r++) B[c + 4][row + r] = buffer[r];
	          for (r = 4; r < 8; r++) B[c + 4][row + r] = A[row + r][c + 4];
	        }
	      }
	    }
  
	如下示意: 
	
	![](https://i.imgur.com/NE00bPi.png)   
	经过step 1: 
	会将第一行的```1 2 3 4 5 6 7 8```保存进```buffer```. 
	经过step 2:
	会将```buffer```中的前4个放在B中的前列,后4个放在B中的前+4列.如下图为B:     
	![](https://i.imgur.com/uv9l7oT.png)   
	经过step3: 
	会将此时B中的后4个元素保存进buffer,即```5 13 21 29```会保存进入```buffer```.  
	经过step4: 
	会将此时B中的右上角的4*4的元素替换为A的转置,即如下图所示:  
	![](https://i.imgur.com/J6LgSOJ.png)  
	要注意此时被替换的元素已经保存至```buffer```中了.   
	经过step5: 
	会将上一个```buffer```中的元素继续摆在B的左下角.如下图所示:  
	![](https://i.imgur.com/KoxiaiQ.png)  
	经过step6: 
	会完成剩下的右下角部分的转置,如图所示: 
	![](https://i.imgur.com/DhQCtDv.png)  
	这样就完成了转置.    
	
	我们分析为何能减少miss数:  
	对于step1和step2 我们将元素暂存入```buffer```,并赋给B矩阵的4*8,这样可以比默认的8*4的直接转置减少一半miss. 
	对于step4 分块为4*4,实际上后4列也会存入cache中(根据空间局部性).而step3中将原本应该放在B的左下角的元素也放入进```buffer```中,在step5中使用,这一步也减少了很多miss.   
	故综上,减少了miss.   
	
	结果如下:  
	![](https://i.imgur.com/6FZRJHy.png)  

- M == 61 N == 67 
这种情况与32*32没有区别,只是块大小我们改为16*16即可. 结果如下: 
![](https://i.imgur.com/qEuDIwX.png) 

至此,PartB结束.