# Shell-lab
---------------------
## 实验目的
- 运用unix关于进程的知识写一个shell
## 实验材料及准备
- 修改```tsh.c```文件
- 用到```fork(), kill(), waitpid()......```等系统函数.
## 实验过程
### 概览 
```main()```函数中,首先解析命令行参数,在大循环中最核心的函数为```eval()```函数,它用来执行```内置命令```或者执行```可执行文件```.```内置命令```包括```quit,jobs,bg,fg```.执行他们时都是放在前台运行的.而对于```可执行文件```,我们开启一个**子进程**执行他们,同时我们将每个进程都存在```job_t```数据结构的数组中,```jid```为任务id,```pid```是真正的进程id.


### 实现目标 
- quit :shell的退出
- jobs :列出任务
- bg fg:进程的前后台切换
- 对于信号SIGINT SIGSTP SIGCHLD的处理函数
- 对于进程的正确回收
### 实现步骤
1. 内置命令的实现:
    - quit: 
      ```
      if (!strcmp(argv[0], "quit")) {
      exit(0);
      }
      ```
       直接```exit```不要```return```
    - jobs:
    ```listjobs```,列出进程状态:```Running``` , ```Foreground```,```Stopped```.
    - bg fg 
    切换进程前后台:```do_bgfg()```
      ```
         if (strcmp(argv[0], "bg") == 0) {
         givenjob->state = BG;
         printf("[%d] (%d) %s", givenjob->jid, givenjob->pid, givenjob->cmdline);
         Kill(-(givenjob->pid), SIGCONT);
       } else {
         givenjob->state = FG;
         printf("[%d] (%d) %s", givenjob->jid, givenjob->pid, givenjob->cmdline);
         Kill(-(givenjob->pid), SIGCONT);
         // waitfg means wait the fgjob untill it finish.
         // waitfg use loop to makesure fg
         waitfg(givenjob->pid);
       }
       ```
       即切换时首先将线程暂停,即```Ctrl+Z```,再改变该线程的状态,之后再用Kill()发送```SIGCONT```信号继续运行.
       对于变为前台进程时,我们使用```Waitfg()```函数进行等待前台进程结束.
2. 对于执行```可执行文件```的处理
    ```
     if (!builtin_cmd(args)) {
     Sigemptyset(&mask);
     Sigaddset(&mask, SIGCHLD);
     Sigprocmask(SIG_BLOCK, &mask, NULL); /* block SIGCHILD */

     // child process
     if ((pid = Fork()) == 0) {
       Setpgid(0, 0); /* If not setpgid(0,0) , ctrl+c will shut down your shell.
                         (because SIGINT will be sent to every fg process in the
                         group) , so set every process different group*/
       Sigprocmask(SIG_UNBLOCK, &mask, NULL); /* unblock SIGCHLD */
       if (execve(args[0], args, environ) < 0) {
         printf("%s: Command not found\n", args[0]);
         exit(0);
       }
     }

     // parent process
     if (!bg) { /* foreground process */
       addjob(jobs, pid, FG, cmdline);
       Sigprocmask(SIG_UNBLOCK, &mask, NULL); /* unblock SIGCHLD after add job*/
       waitfg(pid);
     } else { /* background process */
       addjob(jobs, pid, BG, cmdline);
       Sigprocmask(SIG_UNBLOCK, &mask, NULL); /* unblock SIGCHLD after add job*/
       printf("[%d] (%d) %s", pid2jid(pid), (int)pid, cmdline);
     }
    }
    ```

    此处我们使用```sigaddset```和```sigprocmask```进行屏蔽```SIGCHLD```信号,这样做的目的是,如果我们再创建进程时接受到了```SIGCHLD```信号,会首先执行信号处理函数,我们并不希望这样,所以屏蔽.同样在```addjob```时,也不能接受```SIGCHLD```信号,执行处理函数,这个处理函数会将原本的job从列表中删除,造成先删除后添加的后果.
    
    还要注意的是,要将添加```Setpgid(0,0)```,即每创建一个进程,都放入单独的进程组中.如果没有这一句,在shell中创建的每一个进程都是和shell本身一个线程组,这样Ctrl+C时,也会关闭shell.
  3. 对于SIGINT SIGTSTP SIGCHLD信号的处理
  ```sigchld_handler```:
     ```
     while ((pid = Waitpid(-1, &status, WNOHANG | WUNTRACED)) > 0) {
      jid = pid2jid(pid);

      // return true if the childprocess terminated normally
      if (WIFEXITED(status)) {
        deletejob(jobs, pid);
        if (verbose)
          printf("sigchld_handler: Job [%d] (%d) deleted\n", jid, (int)pid);
        if (verbose)
          printf("sigchld_handler: Job [%d] (%d) terminates OK (status %d)\n",
                 jid, (int)pid, WEXITSTATUS(status));
      } else if (WIFSIGNALED(status)) {
        // return true if the childprocess terminated because of a signal that
        // was not caught
        deletejob(jobs, pid);
        if (verbose)
          printf("sigchld_handler: Job [%d] (%d) deleted\n", jid, (int)pid);
        // WTERMSIG(status) return the real signal that cause terminate.
        printf("Job [%d] (%d) terminated by signal %d\n", jid, (int)pid,
               WTERMSIG(status));
      } else if (WIFSTOPPED(status)) {
        // return true if the signal is stppped
        getjobpid(jobs, pid)->state = ST;  // change job status to STOPPED
        printf("Job [%d] (%d) stopped by signal %d\n", jid, (int)pid,
               WSTOPSIG(status));
      }
     }
     ```
    
     对于```sigtstp_handler```和```sigint_handler```都是利用```kill()```发送信号.
     此处注意,```kill(-pid,sig)```第一个参数是负数,对该进程组的所有进程发送信号!

4. 常见问题
    - 从标准Unix shell运行shell时，shell正在前台进程组中运行。 如果您的shell然后创建子进程，则默认情况下该子进程也将是前台进程组的成员。 
   由于输入ctrl-c会向前台组中的每个进程发送一个SIGINT，因此键入ctrl-c会将SIGINT发送到shell，以及shell创建的每个进程，这显然是不正确的。
   也就是说会将你的shell创建了前台和多个后台进程,此时发送一个ctrl-c会将前台后台进程都杀死,我们需要的只是杀死前台即可.(而在内核看来,由于
   你的shell是一个前台进程,他再创建的进程(对于他本身分为前台后台)都是这个前台进程组的进程,所以一并杀死).
   
      解决方法: 在fork之后但在execve之前，子进程应该调用setpgid（0,0），它将子进程放入一个新的进程组,其组ID与子进程的PID相同。 这可确保前台进程组中只有一个进程shell. 当您键入ctrl-c时，shell应捕获生成的SIGINT，然后将其转发到相应的前台作业（或更准确地说，包含前台作业的进程组）
    
    - 对于包装函数Waitpid()来说,我们不能再使用if(waitpid(pid iptr,options)<0) unix_error(...);因为当所有的子进程都终止后,这个waitpid()会返回-1 并且此时设置errorno为1,即进入unix_error后,会输出"no child processes"错误信息,并exit(1),这样的话就导致我们的shell终止了.以改成if();即可.
   

    
