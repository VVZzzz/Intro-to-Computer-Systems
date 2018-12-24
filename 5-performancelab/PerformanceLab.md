# PerformanceLab
------------------------------
## 实验目的:
运用CodeOptimization技巧进行代码优化
## 实验准备:
- 循环展开 ( K*1 K*M展开等)
- 使用局部变量保存,避免多次调用函数等技巧
## 注意 
该lab在2014年后已被移除,内容不多也比较水.

## 实验过程
修改```kernel.c```文件中,```void rotate()```和```void smooth()```函数即可.   
并使用命令```./driver -t ```进行测试. 

- 优化```void rotate()```
原函数为:

		void naive_rotate(int dim, pixel *src, pixel *dst) {
		  int i, j;
		
		  for (i = 0; i < dim; i++)
		    for (j = 0; j < dim; j++)
		      dst[RIDX(dim - 1 - j, i, dim)] = src[RIDX(i, j, dim)];
		}
我们的优化策略为: 避免多次调用```RIDX```函数,并且进行函数循环展开.经实验,展开为32*1展开比较合理.   
那么优化过后的代码为:  

		void rotate(int dim, pixel *src, pixel *dst) {
		  /*循环展开 block*1 */
		  /*dst : dim*dim - dim - j*dim + i*/
		  /*src : i*dim + j*/
		  const int block = 32;
		  int i, j;
		  int buff = dim * dim;
		  dst += buff - dim;
		  for (i = 0; i < dim; i += 32) {
		    for (j = 0; j < dim; j++) {
		      *dst = *src;
		
		      src += dim;
		      *(dst + 1) = *src;
		
		      ...
			  ...
		
		      src += dim;
		      *(dst + 31) = *src;
		
		      dst -= dim;
		      src -= dim * 31 - 1;
		    }
		    dst += 32 + buff;
		    src += dim * 31;
		  }
		  return;
		}

实验结果如下:  
![](https://i.imgur.com/IIX4MB2.png)  
可以发现,效率提高15.4/6.6倍.    

- 优化smooth()  
原代码为:  

		void naive_smooth(int dim, pixel *src, pixel *dst) {
		  int i, j;
		
		  for (i = 0; i < dim; i++)
		    for (j = 0; j < dim; j++) dst[RIDX(i, j, dim)] = avg(dim, i, j, src);
		}
发现调用函数次数过多,且进行```avg()```求解均值时进行比较多的判断.     
我们优化策略是,不调用函数(牺牲模块性).并进行分类处理, 先进行4个角的像素点,之后是四条边的,最后为中心部分.    
这样可以减少条件转移和计算索引.    

实验结果如下:    
![](https://i.imgur.com/Ilu3jHy.png)    
效率提升了39.2/13.0
