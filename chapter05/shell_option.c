xs#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
/*
 * stdio.h:
 *   fprintf(), fgets(), sscanf() を使用する。
 *
 * stdlib.h:
 *   exit() を使用する。
 *
 * sys/wait.h:
 *   wait() を使用し、子プロセスの終了を回収する。
 *
 * unistd.h:
 *   fork(), execv() を使用する。
 *
 * string.h:
 *   strtok() を使用する（文字列分割）。
 */

int get_arg(char *c, char *arg[]);
/*
 * get_arg():
 *   入力されたコマンド文字列を空白で分割し、
 *   execv() に渡せる argv 形式に変換する関数。
 */

int main(){
   pid_t pid;
   /*
    * fork() の戻り値を保持する。
    * 親では子のPID、子では0。
    */

   int ret, st, i=0;
   /*
    * ret:
    *   execv() の戻り値（成功時は戻らない）。
    *
    * st:
    *   wait() による子の終了ステータス。
    */

   char line[256], command[256], *arg[32];
   /*
    * line:
    *   ユーザー入力全体を保持。
    *
    * command:
    *   改行を除去したコマンド文字列。
    *
    * arg:
    *   execv() に渡す引数配列。
    *   最大31個 + NULL終端。
    */

   while(1){
      /*
       * 無限ループ。
       * 簡易シェルとして繰り返し入力を受け付ける。
       */

      fprintf(stderr,"--> ");
      /*
       * プロンプト表示。
       */

      fgets(line,sizeof(line),stdin);
      /*
       * 1行分の入力を取得。
       * 例: "/bin/ls -l"
       */

      sscanf(line,"%[^\n]",command);
      /*
       * 改行を除去して command にコピー。
       */

      get_arg(command, arg);
      /*
       * command を空白で分割。
       *
       * 例:
       *   command = "/bin/ls -l"
       *
       *   arg[0] = "/bin/ls"
       *   arg[1] = "-l"
       *   arg[2] = NULL
       *
       * execv() にそのまま渡せる形式になる。
       */

      pid = fork();
      /*
       * プロセスを複製。
       */

      if(pid == 0){
         /*
          * 子プロセス側。
          */

         ret = execv(arg[0], arg);
         /*
          * execv():
          *   現在のプロセスを arg[0] で指定されたプログラムに置換。
          *
          * 引数形式:
          *   execv(path, argv)
          *
          * argv は:
          *   argv[0] = プログラム名
          *   argv[n] = 引数
          *   最後は NULL
          *
          * 成功すれば戻らない。
          */

         if(ret < 0){
            /*
             * execv が失敗した場合のみここに来る。
             * 例:
             *   実行ファイルが存在しない
             *   実行権限がない
             */

            exit(0);
            /*
             * 子プロセス終了。
             */
         }
      }
      else{
         /*
          * 親プロセス側。
          */

         wait(&st);
         /*
          * 子の終了を待つ。
          *
          * これにより:
          *   - ゾンビプロセスを防ぐ
          *   - コマンド実行が終わるまで次の入力を受け付けない
          */
      }
   }

   return 0;
   /*
    * 実質的に到達しない。
    */
}


/*
 * get_arg():
 *
 * 入力文字列を strtok() によって空白区切りで分割する。
 *
 * strtok() の動作:
 *   - 最初の呼び出しで文字列を分割
 *   - 以降 NULL を渡すことで続きから分割
 *
 * 注意:
 *   strtok() は元の文字列を破壊的に変更する。
 */
int get_arg(char *c, char *arg[]){
   int i=0;

   arg[i] = strtok(c," ");
   /*
    * 最初のトークンを取得。
    */

   while(arg[i] != NULL){
      i++;
      arg[i] = strtok(NULL," ");
      /*
       * 続きのトークンを取得。
       * トークンがなくなると NULL が返る。
       */
   }

   return i;
   /*
    * 分割された引数数を返す（NULLは含まない）。
    */
}
