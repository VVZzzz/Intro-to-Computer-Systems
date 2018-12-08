# BombLab
------------------------------
## 实验目的:
运用GDB和反汇编工具,将bomb可执行文件中的6个phase的答案找到.
## 实验准备:
- GDB调试工具
- GDB调试常用命令

## 实验过程

- ```objdump -d ./bomb > assemble.txt``` 将反汇编得到的代码输入到```assemble.txt```中.

	得到如下代码,在调试中可以参考查看:

		0000000000400da0 <main>:
		  400da0:	53                   	push   %rbx
		  400da1:	83 ff 01             	cmp    $0x1,%edi
		  400da4:	75 10                	jne    400db6 <main+0x16>
		  400da6:	48 8b 05 9b 29 20 00 	mov    0x20299b(%rip),%rax        # 603748 <stdin@@GLIBC_2.2.5>
		  400dad:	48 89 05 b4 29 20 00 	mov    %rax,0x2029b4(%rip)        # 603768 <infile>
		  400db4:	eb 63                	jmp    400e19 <main+0x79>
		  400db6:	48 89 f3             	mov    %rsi,%rbx
		  400db9:	83 ff 02             	cmp    $0x2,%edi
		  400dbc:	75 3a                	jne    400df8 <main+0x58>
		  400dbe:	48 8b 7e 08          	mov    0x8(%rsi),%rdi
		  400dc2:	be b4 22 40 00       	mov    $0x4022b4,%esi
		  400dc7:	e8 44 fe ff ff       	callq  400c10 <fopen@plt>
		  400dcc:	48 89 05 95 29 20 00 	mov    %rax,0x202995(%rip)        # 603768 <infile>
		  400dd3:	48 85 c0             	test   %rax,%rax
		  400dd6:	75 41                	jne    400e19 <main+0x79>
		  400dd8:	48 8b 4b 08          	mov    0x8(%rbx),%rcx
		  400ddc:	48 8b 13             	mov    (%rbx),%rdx
		  400ddf:	be b6 22 40 00       	mov    $0x4022b6,%esi
		  400de4:	bf 01 00 00 00       	mov    $0x1,%edi
		  400de9:	e8 12 fe ff ff       	callq  400c00 <__printf_chk@plt>
		  400dee:	bf 08 00 00 00       	mov    $0x8,%edi
		  400df3:	e8 28 fe ff ff       	callq  400c20 <exit@plt>
		  400df8:	48 8b 16             	mov    (%rsi),%rdx
		  400dfb:	be d3 22 40 00       	mov    $0x4022d3,%esi
		  400e00:	bf 01 00 00 00       	mov    $0x1,%edi
		  400e05:	b8 00 00 00 00       	mov    $0x0,%eax
		  400e0a:	e8 f1 fd ff ff       	callq  400c00 <__printf_chk@plt>
		  400e0f:	bf 08 00 00 00       	mov    $0x8,%edi
		  400e14:	e8 07 fe ff ff       	callq  400c20 <exit@plt>
		  400e19:	e8 84 05 00 00       	callq  4013a2 <initialize_bomb>
		  400e1e:	bf 38 23 40 00       	mov    $0x402338,%edi
		  400e23:	e8 e8 fc ff ff       	callq  400b10 <puts@plt>
		  400e28:	bf 78 23 40 00       	mov    $0x402378,%edi
		  400e2d:	e8 de fc ff ff       	callq  400b10 <puts@plt>
		  400e32:	e8 67 06 00 00       	callq  40149e <read_line>
		  400e37:	48 89 c7             	mov    %rax,%rdi
		  400e3a:	e8 a1 00 00 00       	callq  400ee0 <phase_1>
		  400e3f:	e8 80 07 00 00       	callq  4015c4 <phase_defused>
		  400e44:	bf a8 23 40 00       	mov    $0x4023a8,%edi
		  400e49:	e8 c2 fc ff ff       	callq  400b10 <puts@plt>
		  400e4e:	e8 4b 06 00 00       	callq  40149e <read_line>
		  400e53:	48 89 c7             	mov    %rax,%rdi
		  400e56:	e8 a1 00 00 00       	callq  400efc <phase_2>
		  400e5b:	e8 64 07 00 00       	callq  4015c4 <phase_defused>
		  400e60:	bf ed 22 40 00       	mov    $0x4022ed,%edi
		  400e65:	e8 a6 fc ff ff       	callq  400b10 <puts@plt>
		  400e6a:	e8 2f 06 00 00       	callq  40149e <read_line>
		  400e6f:	48 89 c7             	mov    %rax,%rdi
		  400e72:	e8 cc 00 00 00       	callq  400f43 <phase_3>
		  400e77:	e8 48 07 00 00       	callq  4015c4 <phase_defused>
		  400e7c:	bf 0b 23 40 00       	mov    $0x40230b,%edi
		  400e81:	e8 8a fc ff ff       	callq  400b10 <puts@plt>
		  400e86:	e8 13 06 00 00       	callq  40149e <read_line>
		  400e8b:	48 89 c7             	mov    %rax,%rdi
		  400e8e:	e8 79 01 00 00       	callq  40100c <phase_4>
		  400e93:	e8 2c 07 00 00       	callq  4015c4 <phase_defused>
		  400e98:	bf d8 23 40 00       	mov    $0x4023d8,%edi
		  400e9d:	e8 6e fc ff ff       	callq  400b10 <puts@plt>
		  400ea2:	e8 f7 05 00 00       	callq  40149e <read_line>
		  400ea7:	48 89 c7             	mov    %rax,%rdi
		  400eaa:	e8 b3 01 00 00       	callq  401062 <phase_5>
		  400eaf:	e8 10 07 00 00       	callq  4015c4 <phase_defused>
		  400eb4:	bf 1a 23 40 00       	mov    $0x40231a,%edi
		  400eb9:	e8 52 fc ff ff       	callq  400b10 <puts@plt>
		  400ebe:	e8 db 05 00 00       	callq  40149e <read_line>
		  400ec3:	48 89 c7             	mov    %rax,%rdi
		  400ec6:	e8 29 02 00 00       	callq  4010f4 <phase_6>
		  400ecb:	e8 f4 06 00 00       	callq  4015c4 <phase_defused>
		  400ed0:	b8 00 00 00 00       	mov    $0x0,%eax
		  400ed5:	5b                   	pop    %rbx
		  400ed6:	c3                   	retq   

- ``` gdb bomb```进入gdb调试,并使用```layout asm```进入TUI模式
![](https://i.imgur.com/ZpTVJoM.png)

### phase_1
```disassemble phase_1```对phase_1反汇编.

	0000000000400ee0 <phase_1>:
	  400ee0:	48 83 ec 08          	sub    $0x8,%rsp
	  400ee4:	be 00 24 40 00       	mov    $0x402400,%esi
	  400ee9:	e8 4a 04 00 00       	callq  401338 <strings_not_equal>  //将rsi和rdi中的字符串进行比较
	  400eee:	85 c0                	test   %eax,%eax
	  400ef0:	74 05                	je     400ef7 <phase_1+0x17>
	  400ef2:	e8 43 05 00 00       	callq  40143a <explode_bomb>
	  400ef7:	48 83 c4 08          	add    $0x8,%rsp
	  400efb:	c3                   	retq   
故我们查看0x402400单元的字符串值即可.
![](https://i.imgur.com/gVPz0ay.png)
phase_1答案为
```Border relations with Canada have never been better.```

### phase_2

	 Dump of assembler code for function phase_2:
       0x0000000000400efc <+0>: push   %rbp
       0x0000000000400efd <+1>: push   %rbx
       0x0000000000400efe <+2>: sub    $0x28,%rsp
       0x0000000000400f02 <+6>: mov    %rsp,%rsi
       0x0000000000400f05 <+9>: callq  0x40145c <read_six_numbers> //读入6个数字放在rsp单元,不是则爆炸
       0x0000000000400f0a <+14>:    cmpl   $0x1,(%rsp) //判断第一个数字是否为1
       0x0000000000400f0e <+18>:    je     0x400f30  <phase_2+52> //为1则进入循环
       0x0000000000400f10 <+20>:    callq  0x40143a <explode_bomb>//不是1，则爆炸
       0x0000000000400f15 <+25>:    jmp    0x400f30 <phase_2+52>
       0x0000000000400f17 <+27>:    mov    -0x4(%rbx),%eax 
       0x0000000000400f1a <+30>:    add    %eax,%eax
       0x0000000000400f1c <+32>:    cmp    %eax,(%rbx) //后一个元素是否为前一个元素的2倍
       0x0000000000400f1e <+34>:    je     0x400f25 <phase_2+41>
       0x0000000000400f20 <+36>:    callq  0x40143a <explode_bomb> //不是两倍则爆炸
       0x0000000000400f25 <+41>:    add    $0x4,%rbx
       0x0000000000400f29 <+45>:    cmp    %rbp,%rbx
       0x0000000000400f2c <+48>:    jne    0x400f17 <phase_2+27>
       0x0000000000400f2e <+50>:    jmp    0x400f3c <phase_2+64>
       0x0000000000400f30 <+52>:    lea    0x4(%rsp),%rbx
       0x0000000000400f35 <+57>:    lea    0x18(%rsp),%rbp
       0x0000000000400f3a <+62>:    jmp    0x400f17 <phase_2+27>
       0x0000000000400f3c <+64>:    add    $0x28,%rsp
       0x0000000000400f40 <+68>:    pop    %rbx
       0x0000000000400f41 <+69>:    pop    %rbp
       0x0000000000400f42 <+70>:    retq   
    End of assembler dump.

根据分析,输入六个数,第一个数为1,且之后的每个数都是前一个数的2倍.故答案为:
```1 2 4 8 16 32```

### phase_3

	0000000000400f43 <phase_3>:
	  400f43:	sub    $0x18,%rsp
	  400f47:	lea    0xc(%rsp),%rcx
	  400f4c:	lea    0x8(%rsp),%rdx
	  400f51:	mov    $0x4025cf,%esi  //查看0x4025cf单元为"%d %d"
	  400f56:	mov    $0x0,%eax
	  400f5b:	callq  400bf0 <__isoc99_sscanf@plt>
	  400f60:	cmp    $0x1,%eax  //eax保存scanf的返回值,即输入的个数
	  400f63:	jg     400f6a <phase_3+0x27>
	  400f65:	callq  40143a <explode_bomb>
	  400f6a:	cmpl   $0x7,0x8(%rsp) //查看第一个数是否大于7,大于7则爆炸
	  400f6f:	ja     400fad <phase_3+0x6a>
	  400f71:	mov    0x8(%rsp),%eax
	  400f75:	jmpq   *0x402470(,%rax,8)
	  400f7c:	mov    $0xcf,%eax
	  400f81:	jmp    400fbe <phase_3+0x7b>
	  400f83:	mov    $0x2c3,%eax
	  400f88:	jmp    400fbe <phase_3+0x7b>
	  400f8a:	mov    $0x100,%eax
	  400f8f:	jmp    400fbe <phase_3+0x7b>
	  400f91:	mov    $0x185,%eax
	  400f96:	jmp    400fbe <phase_3+0x7b>
	  400f98:	mov    $0xce,%eax
	  400f9d:	jmp    400fbe <phase_3+0x7b>
	  400f9f:	mov    $0x2aa,%eax
	  400fa4:	jmp    400fbe <phase_3+0x7b>
	  400fa6:	mov    $0x147,%eax
	  400fab:	jmp    400fbe <phase_3+0x7b>
	  400fad:	callq  40143a <explode_bomb>
	  400fb2:	mov    $0x0,%eax
	  400fb7:	jmp    400fbe <phase_3+0x7b>
	  400fb9:	mov    $0x137,%eax
	  400fbe:	cmp    0xc(%rsp),%eax
	  400fc2:	je     400fc9 <phase_3+0x86>
	  400fc4:	callq  40143a <explode_bomb>
	  400fc9:	add    $0x18,%rsp
	  400fcd:	retq   

故我们分析这个代码:  
```printf (char*) 0x4025cf```得到```"%d %d"```即   
首先scanf读入两个整数,如果第一个数>7 则爆炸,说明第一个数必须≤7.  
之后剩下的代码类似一个```switch```,通过第一个参数,来进行跳转.比如如果第一个参数为0,则进行:  
   
	400f7c:	mov    $0xcf,%eax
	  400f81:	jmp    400fbe <phase_3+0x7b>
跳转得到的语句为:比较第二个参数是否和```%eax相等```

故我们可以得到答案必须是如下几组答案 :

    0 207 
    1 311  
    2 707  
    3 256  
    4 389  
    5 206  
    6 682  
    7 327  

### phase_4

	000000000040100c <phase_4>:
	  40100c:	sub    $0x18,%rsp
	  401010:	lea    0xc(%rsp),%rcx
	  401015:	lea    0x8(%rsp),%rdx
	  40101a:	mov    $0x4025cf,%esi
	  40101f:	mov    $0x0,%eax
	  401024:	callq  400bf0 <__isoc99_sscanf@plt>
	  401029:	cmp    $0x2,%eax                    //需要输入两个数字 
	  40102c:	jne    401035 <phase_4+0x29>
	  40102e:	cmpl   $0xe,0x8(%rsp)               //第一个数字应该≤14
	  401033:	jbe    40103a <phase_4+0x2e>
	  401035:	callq  40143a <explode_bomb>
	  40103a:	mov    $0xe,%edx
	  40103f:	mov    $0x0,%esi
	  401044:	mov    0x8(%rsp),%edi				//func4有三个参数 edx esi 第一个数字(edi)
	  401048:	callq  400fce <func4>
	  40104d:	test   %eax,%eax					//func4的返回值%eax必须为0,否则爆炸
	  40104f:	jne    401058 <phase_4+0x4c>
	  401051:	cmpl   $0x0,0xc(%rsp)				//第二个数字必须为0
	  401056:	je     40105d <phase_4+0x51>
	  401058:	callq  40143a <explode_bomb>
	  40105d:	add    $0x18,%rsp
	  401061:	retq   

查看func4的反汇编代码: ```func4(edx=14, esi=0, edi=para1)```

	400fce:	sub    $0x8,%rsp
	400fd2:	mov    %edx,%eax
	400fd4:	sub    %esi,%eax
	400fd6:	mov    %eax,%ecx			//ecx=eax=(edx-esi)
	400fd8:	shr    $0x1f,%ecx			//ecx>>=31
	400fdb:	add    %ecx,%eax			//eax=eax+ecx
	400fdd:	sar    %eax					//eax>>1,所以eax现在为(edx-esi)/2
	400fdf:	lea    (%rax,%rsi,1),%ecx	//ecx=rax+rsi+1
	400fe2:	cmp    %edi,%ecx
	400fe4:	jle    400ff2 <func4+0x24>
	400fe6:	lea    -0x1(%rcx),%edx
	400fe9:	callq  400fce <func4>
	400fee:	add    %eax,%eax
	400ff0:	jmp    401007 <func4+0x39>
	400ff2:	mov    $0x0,%eax
	400ff7:	cmp    %edi,%ecx
	400ff9:	jge    401007 <func4+0x39>
	400ffb:	lea    0x1(%rcx),%esi
	400ffe:	callq  400fce <func4>
	401003:	lea    0x1(%rax,%rax,1),%eax
	401007:	add    $0x8,%rsp
	40100b:	retq   
我们还原func4:
	
	func4(edx=14,esi=0,edi=para1)
	{
		ecx=(edx-esi)/2+esi+1;
		eax=(edx-esi)/1;
		if(edi <= ecx)
		{
			eax=0;
			if(edi >= ecx)
				return eax;
			else
			{
				esi=ecx+1;
				return 2*func(edx,esi,edi)+1;
			}
		}
		else
		{
			edx=rcx-1;
			return 2*func(edx,esi,edi);
		}
	}

故我们可以看出其中的一组解为: ```7 0```,第一个数字等于14的一半7时,func4可以返回0,不爆炸.   
另一组解为: ```0 0``` 代入同样可以返回0.   

### phase_5

	0000000000401062 <phase_5>:
	  401062:	push   %rbx
	  401063:	sub    $0x20,%rsp
	  401067:	mov    %rdi,%rbx				//将输入字符串放入rbx中
	  40106a:	mov    %fs:0x28,%rax
	  401071:
	  401073:	mov    %rax,0x18(%rsp)
	  401078:	xor    %eax,%eax
	  40107a:	callq  40131b <string_length>
	  40107f:	cmp    $0x6,%eax				//字符串长度为6,不为6则爆炸
	  401082:	je     4010d2 <phase_5+0x70>
	  401084:	callq  40143a <explode_bomb>
	  401089:	jmp    4010d2 <phase_5+0x70>
	  40108b:	movzbl (%rbx,%rax,1),%ecx		//从rbx字符串中取字符到edx中
	  40108f:	mov    %cl,(%rsp)
	  401092:	mov    (%rsp),%rdx
	  401096:	and    $0xf,%edx				//edx中取最后四位 0-15
	  401099:	movzbl 0x4024b0(%rdx),%edx		//根据这四位表示的数字,取0x4024b0字符串中的对应的字符

	  											//"maduiersnfotvbylSo you think you can stop the bomb with ctrl-c, do you?"

	  4010a0:	mov    %dl,0x10(%rsp,%rax,1)
	  4010a4:	add    $0x1,%rax				//计数器,控制循环6次
	  4010a8:	cmp    $0x6,%rax
	  4010ac:	jne    40108b <phase_5+0x29>
	  4010ae:	movb   $0x0,0x16(%rsp)			//添加'\0'
	  4010b3:	mov    $0x40245e,%esi			//0x40245e单元中的内容为"flyers"
	  4010b8:	lea    0x10(%rsp),%rdi			//strings_not_equal比较字符串
	  4010bd:	callq  401338 <strings_not_equal>
	  4010c2:	test   %eax,%eax
	  4010c4:	je     4010d9 <phase_5+0x77>
	  4010c6:	callq  40143a <explode_bomb>
	  4010cb:	nopl   0x0(%rax,%rax,1)
	  4010d0:	jmp    4010d9 <phase_5+0x77>
	  4010d2:	mov    $0x0,%eax				
	  4010d7:	jmp    40108b <phase_5+0x29>
	  4010d9:	mov    0x18(%rsp),%rax
	  4010de:	xor    %fs:0x28,%rax
	  4010e5:
	  4010e7:	je     4010ee <phase_5+0x8c>
	  4010e9:	callq  400b30 <__stack_chk_fail@plt>
	  4010ee:	add    $0x20,%rsp
	  4010f2:	pop    %rbx
	  4010f3:	retq   

故分析以上代码,大致意思是我们输入一个字符串长度为6的字符串,对每一个字符的ASCII码取最后四位.   
以此为索引,找到```0x4024b0```单元中字符串```maduiersnfotvbylSo you think you can stop the bomb with ctrl-c, do you?```对应的字符,并组成一个字符串,检查这个字符串是否和```flyers```相等.  
![](https://i.imgur.com/UTG2PtZ.png)  

故```flyers```每个字符对应的ASCII值的后四位为 9 15 14 5 6 7,故其中一个答案为:
```ionefg（0x49，0x4f,0x4E,0x45,0x46,0x47）```.

### phase_6
	
	  4010f4:	push   %r14
	  4010f6:	push   %r13
	  4010f8:	push   %r12
	  4010fa:	push   %rbp
	  4010fb:	push   %rbx							//入栈保存寄存器状态

	  4010fc:	sub    $0x50,%rsp
	  401100:	mov    %rsp,%r13
	  401103:	mov    %rsp,%rsi
	  401106:	callq  40145c <read_six_numbers>	//读入6个数字,放入rsp,rsp+4,.....中
	  40110b:	mov    %rsp,%r14

	  40110e:	mov    $0x0,%r12d					//r12d用来计数
	  401114:	mov    %r13,%rbp
	  401117:	mov    0x0(%r13),%eax
	  40111b:	sub    $0x1,%eax
	  40111e:	cmp    $0x5,%eax
	  401121:	jbe    401128 <phase_6+0x34>
	  401123:	callq  40143a <explode_bomb>
	  401128:	add    $0x1,%r12d
	  40112c:	cmp    $0x6,%r12d					//循环6此
	  401130:	je     401153 <phase_6+0x5f>
	  401132:	mov    %r12d,%ebx
	  401135:	movslq %ebx,%rax					//7-input[i]
	  401138:	mov    (%rsp,%rax,4),%eax			//input[i] = 7 - input[i]
	  40113b:	cmp    %eax,0x0(%rbp)				//要求输入的6个元素不能等于0
	  40113e:	jne    401145 <phase_6+0x51>
	  401140:	callq  40143a <explode_bomb>
	  401145:	add    $0x1,%ebx
	  401148:	cmp    $0x5,%ebx
	  40114b:	jle    401135 <phase_6+0x41>
	  40114d:	add    $0x4,%r13
	  401151:	jmp    401114 <phase_6+0x20>
	  401153:	lea    0x18(%rsp),%rsi
	  401158:	mov    %r14,%rax
	  40115b:	mov    $0x7,%ecx
	  401160:	mov    %ecx,%edx
	  401162:	sub    (%rax),%edx
	  401164:	mov    %edx,(%rax)
	  401166:	add    $0x4,%rax
	  40116a:	cmp    %rsi,%rax
	  40116d:	jne    401160 <phase_6+0x6c>

接下来是将我们输入的6个数字比如: 4 3 2 1 6 5,按照元素值大小顺序放入内存单元0x6032d0中,由于第一个元素是4,那么第4个元素应该排在0x6032d0处的第一个位置.

	  40116f:	mov    $0x0,%esi
	  401174:	jmp    401197 <phase_6+0xa3>
	  401176:	mov    0x8(%rdx),%rdx				//找到第ecx个node
	  40117a:	add    $0x1,%eax
	  40117d:	cmp    %ecx,%eax
	  40117f:	jne    401176 <phase_6+0x82>
	  401181:	jmp    401188 <phase_6+0x94>
	  401183:	mov    $0x6032d0,%edx
	  401188:	mov    %rdx,0x20(%rsp,%rsi,2)		//将相应的node放入指定位置
	  40118d:	add    $0x4,%rsi					//rsi用来指向下一个元素
	  401191:	cmp    $0x18,%rsi					//防止越界
	  401195:	je     4011ab <phase_6+0xb7>
	  401197:	mov    (%rsp,%rsi,1),%ecx
	  40119a:	cmp    $0x1,%ecx
	  40119d:	jle    401183 <phase_6+0x8f>
	  40119f:	mov    $0x1,%eax
	  4011a4:	mov    $0x6032d0,%edx
	  4011a9:	jmp    401176 <phase_6+0x82>

将node之间建立链表

	  4011ab:	mov    0x20(%rsp),%rbx
	  4011b0:	lea    0x28(%rsp),%rax
	  4011b5:	lea    0x50(%rsp),%rsi
	  4011ba:	mov    %rbx,%rcx
	  4011bd:	mov    (%rax),%rdx
	  4011c0:	mov    %rdx,0x8(%rcx)				//将下一个Node地址赋给上一个Node
	  4011c4:	add    $0x8,%rax
	  4011c8:	cmp    %rsi,%rax
	  4011cb:	je     4011d2 <phase_6+0xde>
	  4011cd:	mov    %rdx,%rcx
	  4011d0:	jmp    4011bd <phase_6+0xc9>
	  4011d2:	movq   $0x0,0x8(%rdx)				//最后一个指针为nullptr
	  4011d9:

判断链表是否递减,若不是则爆炸

	  4011da:	mov    $0x5,%ebp
	  4011df:	mov    0x8(%rbx),%rax
	  4011e3:	mov    (%rax),%eax
	  4011e5:	cmp    %eax,(%rbx)
	  4011e7:	jge    4011ee <phase_6+0xfa>
	  4011e9:	callq  40143a <explode_bomb>
	  4011ee:	mov    0x8(%rbx),%rbx
	  4011f2:	sub    $0x1,%ebp
	  4011f5:	jne    4011df <phase_6+0xeb>
	  4011f7:	add    $0x50,%rsp
	  4011fb:	pop    %rbx
	  4011fc:	pop    %rbp
	  4011fd:	pop    %r12
	  4011ff:	pop    %r13
	  401201:	pop    %r14
	  401203:	retq   

故分析上述代码,我们知道,我们输入input[0]...[5]是为了将链表排序,比如 3 2 4 1 5 6,那么意思就是第3个元素应该排第一个最大,依次是第2个元素...    
但别忘了里面要input[i]=7-input[i].   
我们```stepi```逐步调试:  (输入 1 2 3 4 5 6)
如下图设置断点:
![](https://i.imgur.com/mGH0CrU.png)     
读入6个数,放在%rsp中:     
![](https://i.imgur.com/bPA6Vrp.png)      
之后我们在 ```0x4011da```处设置断点,即此时应该是已经建立好链表的一个状态.       
用```x/wd 0x6032d0查看我们保存的链表```` :     
![](https://i.imgur.com/iarbwrd.png)     
![](https://i.imgur.com/164G2jv.png)   

可以看到有6个Node,值分别为 ```322 168 924 691 477 443```    
我们要对他们进行递减排序,故应该为 ```3 4 5 6 1 2```.     
最后进行7-input[i],得到的最终答案为     
```4 3 2 1 6 5```   


### secret_phase
**当然bomb不止六关,还有一个隐藏关卡.**   
反汇编```phase_defused```我们发现一个```secret_phase```.  
![](https://i.imgur.com/5gj7zMW.png)  
首先还是检查```phase_defused```

	00000000004015c4 <phase_defused>:
	  4015c4:	sub    $0x78,%rsp
	  4015c8:	mov    %fs:0x28,%rax
	  4015cf:
	  4015d1:	mov    %rax,0x68(%rsp)
	  4015d6:	xor    %eax,%eax
	  4015d8:	cmpl   $0x6,0x202181(%rip)			# 603760 <num_input_strings> 即是否已经通过了六关
	  4015df:	jne    40163f <phase_defused+0x7b>	
	  4015e1:	lea    0x10(%rsp),%r8
	  4015e6:	lea    0xc(%rsp),%rcx
	  4015eb:	lea    0x8(%rsp),%rdx
	  4015f0:	mov    $0x402619,%esi				# 0x402619的内容是 "%d %d %s" 
	  4015f5:	mov    $0x603870,%edi				# 实际上存放的是phase_4中的输入数据
	  4015fa:	callq  400bf0 <__isoc99_sscanf@plt>
	  4015ff:	cmp    $0x3,%eax					# 是否为3个参数,不等则继续
	  401602:	jne    401635 <phase_defused+0x71>
	  401604:	mov    $0x402622,%esi				# 内容为"DrEvil"
	  401609:	lea    0x10(%rsp),%rdi
	  40160e:	callq  401338 <strings_not_equal>
	  401613:	test   %eax,%eax
	  401615:	jne    401635 <phase_defused+0x71>
	  401617:	mov    $0x4024f8,%edi				# 输出提示
	  40161c:	callq  400b10 <puts@plt>
	  401621:	mov    $0x402520,%edi				# 输出提示
	  401626:	callq  400b10 <puts@plt>
	  40162b:	mov    $0x0,%eax
	  401630:	callq  401242 <secret_phase>		# 跳转隐藏炸弹
	  401635:	mov    $0x402558,%edi
	  40163a:	callq  400b10 <puts@plt>
	  40163f:	mov    0x68(%rsp),%rax
	  401644:	xor    %fs:0x28,%rax
	  40164b:
	  40164d:	je     401654 <phase_defused+0x90>
	  40164f:	callq  400b30 <__stack_chk_fail@plt>
	  401654:	add    $0x78,%rsp
	  401658:	retq  

对于这个程序,实际上如果在```phase_4```中,输入的参数有3个,且最后一个为```"DrEvil"```,则会跳转到隐藏炸弹.
![](https://i.imgur.com/zcLtpv5.png)

![](https://i.imgur.com/A8rRTWk.png)

之后分析```secret_phase```

	  401242:	push   %rbx
	  401243:	callq  40149e <read_line>
	  401248:	mov    $0xa,%edx
	  40124d:	mov    $0x0,%esi
	  401252:	mov    %rax,%rdi
	  401255:	callq  400bd0 <strtol@plt>
	  40125a:	mov    %rax,%rbx
	  40125d:	lea    -0x1(%rax),%eax
	  401260:	cmp    $0x3e8,%eax					# 输入数字,减去1 <=1000
	  401265:	jbe    40126c <secret_phase+0x2a>
	  401267:	callq  40143a <explode_bomb>
	  40126c:	mov    %ebx,%esi
	  40126e:	mov    $0x6030f0,%edi
	  401273:	callq  401204 <fun7>				# 跳转到fun7
	  401278:	cmp    $0x2,%eax					# 返回值为2,则解除炸弹
	  40127b:	je     401282 <secret_phase+0x40>
	  40127d:	callq  40143a <explode_bomb>
	  401282:	mov    $0x402438,%edi
	  401287:	callq  400b10 <puts@plt>
	  40128c:	callq  4015c4 <phase_defused>
	  401291:	pop    %rbx

分析```fun7```:

	  401204:	sub    $0x8,%rsp
	  401208:	test   %rdi,%rdi
	  40120b:	je     401238 <fun7+0x34>
	  40120d:	mov    (%rdi),%edx
	  40120f:	cmp    %esi,%edx
	  401211:	jle    401220 <fun7+0x1c>
	  401213:	mov    0x8(%rdi),%rdi
	  401217:	callq  401204 <fun7>
	  40121c:	add    %eax,%eax
	  40121e:	jmp    40123d <fun7+0x39>
	  401220:	mov    $0x0,%eax
	  401225:	cmp    %esi,%edx
	  401227:	je     40123d <fun7+0x39>
	  401229:	mov    0x10(%rdi),%rdi
	  40122d:	callq  401204 <fun7>
	  401232:	lea    0x1(%rax,%rax,1),%eax
	  401236:	jmp    40123d <fun7+0x39>
	  401238:	mov    $0xffffffff,%eax
	  40123d:	add    $0x8,%rsp
	  401241:	retq   
耐心翻译成C代码: 

	fun7（esi，rdi）{
        if（rdi == 0）return 爆炸

        rdx = (rdi) 内存中的值

        if（esi < edx）{ //我们输入的esi小于第二个参数的值
            rdi = rdi+0x8;
            fun7（esi,rdi）;
            eax = eax*2;
            return ;
        }else{
            eax = 0;
            if(esi == edx) return ;
            rdi = rdi+0x10;
            fun7(esi,edx);
            eax = 1 + eax*2; 
        }
    }

所以我们最后返回结果需要为2,故要首先```esi<edx```,之后进入```esi>edx```,最后```esi==edx```.      
gbd调试,首先查看开始的```rdi```参数为36,故输入的参数要小于36.    
故可以推出,正确答案为```22```.

### 实验结果  
Answer文本如下:   
![](https://i.imgur.com/S63OkLT.png)  
结果如下:
![](https://i.imgur.com/psljxkq.png)
