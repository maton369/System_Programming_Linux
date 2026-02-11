#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
/*
 * stdio.h:
 *   fprintf() を使用するために必要。
 *
 * unistd.h:
 *   fork(), getpid() などのプロセス制御用システムコールを使用。
 *
 * stdlib.h:
 *   exit() を使用するために必要。
 *
 * ※ 本来 wait() を使う場合は <sys/wait.h> を含めるのが正しい。
 */

int main(void){
   pid_t pid;
   /*
    * pid:
    *   fork() の戻り値を格納する変数。
    *
    * fork() の戻り値の意味：
    *   親プロセス  → 子プロセスの PID（正の値）
    *   子プロセス  → 0
    *   エラー時     → -1
    */

   int st;
   /*
    * st:
    *   wait() によって取得する子プロセスの終了ステータス。
    *
    * 子プロセスの exit() の値や終了状態がここに格納される。
    */

   pid = fork();
   /*
    * fork():
    *   現在のプロセスを複製する。
    *
    * 呼び出し後は、
    *   - 親プロセス
    *   - 子プロセス
    * の2つが存在する。
    *
    * 両方のプロセスが fork() の次の行から実行を続ける。
    */

   if(pid == 0){
       /*
        * 子プロセス側の処理。
        * pid == 0 になるのは子だけ。
        */

       fprintf(stderr, "child pid=%d\n", getpid() );
       /*
        * getpid():
        *   現在のプロセスの PID を取得する。
        *
        * ここでは子プロセスの PID が表示される。
        */

       exit(0);
       /*
        * 子プロセスを終了させる。
        *
        * exit(0) は「正常終了」を意味する。
        * この終了コードは親が wait() で取得する。
        */
   }
   else{
       /*
        * 親プロセス側の処理。
        * pid には「子プロセスの PID」が格納されている。
        */

       fprintf(stderr, "parent pid=%d\n", getpid() );
       /*
        * 親自身の PID を表示。
        *
        * 子とは異なる PID になる。
        */

       pid = wait(&st);
       /*
        * wait():
        *   子プロセスの終了を待つ。
        *
        * 引数 &st:
        *   子プロセスの終了情報を格納する。
        *
        * 戻り値:
        *   終了した子プロセスの PID。
        *
        * これにより、
        *   - 親は子が終了するまでブロックされる
        *   - ゾンビプロセスを防ぐ
        */

       fprintf(stderr, "pid= %d st=%d\n", pid, st);
       /*
        * pid:
        *   終了した子プロセスの PID。
        *
        * st:
        *   子の終了状態。
        *
        * 注意：
        *   st はビット構造を持つ値であり、
        *   単純に exit(0) がそのまま入るわけではない。
        *
        * 正しく解釈するには WIFEXITED(), WEXITSTATUS() などを使う。
        */
   }

   return 0;
   /*
    * 親プロセスが正常終了する。
    *
    * 子はすでに exit() で終了している。
    */
}
