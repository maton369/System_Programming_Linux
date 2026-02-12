#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <signal.h>
/*
 * stdio.h:
 *   fprintf(), perror() を使用。
 *
 * stdlib.h:
 *   atoi(), exit() を使用。
 *
 * sys/types.h:
 *   pid_t 型の定義。
 *
 * signal.h:
 *   kill(), SIGKILL などのシグナル定義を使用。
 *
 * このプログラムは、
 *   指定された PID のプロセスに SIGKILL を送信する。
 */

int main(int argc, char *argv[]){

   int ret;
   /*
    * kill() の戻り値を格納。
    * 成功: 0
    * 失敗: -1
    */

   pid_t pid;
   /*
    * pid_t:
    *   プロセスIDを表す型。
    */

   if(argc != 2){
      /*
       * コマンドライン引数が1つ（PID）のみであることを確認。
       *
       * 実行例:
       *   ./a.out 1234
       */
      fprintf(stderr, "Only one PID is required\n");
      exit(1);
   }

   pid = atoi(argv[1]);
   /*
    * 文字列で渡された PID を整数に変換。
    *
    * atoi():
    *   文字列を整数に変換する。
    *
    * 例:
    *   "1234" → 1234
    *
    * 注意:
    *   エラー検出はしない（実務では strtol 推奨）。
    */

   ret = kill(pid, SIGKILL);
   /*
    * kill(pid, signal)
    *
    * 指定された PID のプロセスに
    * シグナルを送信する。
    *
    * SIGKILL:
    *   強制終了シグナル。
    *
    * 特徴:
    *   - 捕捉不可（ハンドラ登録不可）
    *   - 無視不可
    *   - 即時終了
    *
    * カーネルは対象プロセスを即座に終了させる。
    */

   if(ret < 0){
       /*
        * kill が失敗した場合。
        *
        * 主な原因:
        *   - 指定PIDが存在しない
        *   - 権限不足（他ユーザのプロセス）
        */
       perror("kill");
   }

   return 0;
   /*
    * 正常終了。
    */
}
