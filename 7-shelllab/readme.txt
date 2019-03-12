1. 针对./tsh 输入无效command后,会出现tsh> 错位情况.并且无法结束进程.  查找是waitfg中使用死循环+sleep(1)的策略(writeup上推荐的).改成课本上的waitpid
   即可.
2. 参考https://github.com/mightydeveloper/Shell-Lab/blob/master/tsh.c