1. 针对./tsh 输入无效command后,会出现tsh> 错位情况.并且无法结束进程.  
   查找是waitfg中使用死循环+sleep(1)的策略(writeup上推荐的).改成课本上的waitpid
   即可.
2. 参考https://github.com/mightydeveloper/Shell-Lab/blob/master/tsh.c
3. 从标准Unix shell运行shell时，shell正在前台进程组中运行。 如果您的shell然后创建子进程，则默认情况下该子进程也将是前台进程组的成员。 
   由于输入ctrl-c会向前台组中的每个进程发送一个SIGINT，因此键入ctrl-c会将SIGINT发送到shell，以及shell创建的每个进程，这显然是不正确的。
   也就是说会将你的shell创建了前台和多个后台进程,此时发送一个ctrl-c会将前台后台进程都杀死,我们需要的只是杀死前台即可.(而在内核看来,由于
   你的shell是一个前台进程,他再创建的进程(对于他本身分为前台后台)都是这个前台进程组的进程,所以一并杀死).
   
   解决方法: 在fork之后但在execve之前，子进程应该调用setpgid（0,0），它将子进程放入一个新的进程组，
   其组ID与子进程的PID相同。 这可确保前台进程组中只有一个进程shell。 
   当您键入ctrl-c时，shell应捕获生成的SIGINT，然后将其转发到相应的前台作业（或更准确地说，包含前台作业的进程组）。
   
4. 对于包装函数Waitpid()来说,我们不能再使用if(waitpid(pid,iptr,options)<0) unix_error(...);因为当所有的子进程都终止之后,这个waitpid()会返回-1
   并且此时设置errorno为1,即进入unix_error后,会输出"no child processes"错误信息,并exit(1),这样的话就导致我们的shell终止了.所以改成if();即可.