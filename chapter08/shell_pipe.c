#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>

/*
 * このプログラムは「超簡易シェル」を実装している。
 *
 * 対応している機能（アルゴリズム概要）:
 *   - 1コマンドの実行:   /bin/ls -l など（引数付き）
 *   - 2コマンドのパイプ: cmd1 | cmd2 という形（例: ls | wc）
 *
 * 実現手段:
 *   - fork() で子プロセスを作る
 *   - execv() で子プロセスをコマンドに置換する（プロセス置換）
 *   - pipe() で「カーネル内のパイプバッファ」を作り、
 *     dup2() で標準入出力（STDIN/STDOUT）をパイプへ付け替える
 *
 * 重要なUNIXの考え方:
 *   - コマンドは「標準入力から読み、標準出力へ書く」ように作られていることが多い
 *   - パイプは「前のコマンドの標準出力」を「次のコマンドの標準入力」へ接続する
 *
 * 図解（2コマンドパイプの場合）:
 *
 *   親（シェル）
 *      |
 *      | pipe(fd) で通信路を作る
 *      |
 *      +-- fork --> 子1: cmd1
 *      |              close(fd[0])
 *      |              dup2(fd[1], STDOUT)  // cmd1 の出力→パイプへ
 *      |              execv(cmd1,...)
 *      |
 *      +-- fork --> 子2: cmd2
 *                     close(fd[1])
 *                     dup2(fd[0], STDIN)   // cmd2 の入力←パイプから
 *                     execv(cmd2,...)
 *
 *   親は fd[0], fd[1] を閉じて wait を2回行い、子1子2を回収する。
 */

int get_arg2(char *c, char *arg[],char *sym);

int main(){
   pid_t pid;

   char line[256], command[256], *divcom[32], *arg_c1[32], *arg_c2[32];
   /*
    * line:
    *   標準入力から読み込んだ1行（改行含む可能性あり）。
    *
    * command:
    *   line から改行を除いたコマンド文字列を入れるためのバッファ。
    *   sscanf(line,"%[^\n]",command) で改行まで読み取って格納している。
    *
    * divcom:
    *   '|' で分割したコマンド片を格納する配列。
    *   例: "ls -l | wc" → divcom[0]="ls -l ", divcom[1]=" wc"
    *
    * arg_c1, arg_c2:
    *   divcom[0], divcom[1] をさらに空白で分割して execv 用 argv 配列にする。
    *
    * 注意:
    *   get_arg2 は strtok を使うため、分割対象文字列 command/divcom[i] を破壊（\0 を埋める）。
    */

   int st, i=0, ret1, ret2, ret3, ret4, fd[2];
   /*
    * st:
    *   wait() 用の終了ステータス格納。
    *
    * ret1..ret4:
    *   各処理の戻り値格納。
    *
    * fd[2]:
    *   pipe() が返すパイプの FD。
    *     fd[0] = read end
    *     fd[1] = write end
    */

   while(1){
      /*
       * 簡易シェルの REPL ループ:
       *   プロンプト表示 → 入力 → 解析 → 実行 → 次の入力
       */
      fprintf(stderr,"--> ");

      fgets(line, sizeof(line), stdin);
      /*
       * 1行読み込み。
       * EOF の場合は NULL が返るが、このコードは EOF を考慮していない（教材として簡略化）。
       */

      ret1 = sscanf(line, "%[^\n]", command);
      /*
       * "%[^\n]" は「改行以外を全て読む」指定。
       * よって command には改行を除いた文字列が入る。
       *
       * ret1 > 0 のとき入力が取れたと判断する。
       */

      if(ret1 > 0){
         /*
          * まず '|' で分割し、パイプ構文かどうかを判定する。
          */
         ret2 = get_arg2(command, divcom, "|");
         /*
          * get_arg2(command, divcom, "|"):
          *   command を "|" で分割し、divcom にトークン列を入れる。
          *
          * 戻り値 ret2:
          *   分割されたトークン数。
          *
          * ret2 == 1 → '|' がない（単一コマンド）
          * ret2 == 2 → cmd1 | cmd2 の形（2コマンドパイプ）
          *
          * 注意:
          *   strtok は連続呼び出し状態を保持するため、
          *   同時に複数文字列をネストして分割するときは
          *   分割対象の順序や破壊に注意が必要。
          */

         if(ret2 == 1){
            /*
             * 単一コマンドの実行:
             *
             * 例:
             *   "/bin/ls -l"
             *
             * 1) 空白で分割して argv を作る
             * 2) fork() して子で execv()
             * 3) 親は wait() して終了を待つ
             */

            get_arg2(divcom[0], arg_c1, " ");
            /*
             * divcom[0] を空白区切りで分割し、
             * execv(arg_c1[0], arg_c1) の argv 形式にする。
             *
             * 例:
             *   "/bin/ls -l"
             *    arg_c1[0]="/bin/ls"
             *    arg_c1[1]="-l"
             *    arg_c1[2]=NULL
             */

            pid = fork();
            if(pid == 0){
               /*
                * 子プロセス:
                *   execv でコマンドに置換する。
                *
                * execv(path, argv):
                *   現在のプロセスを path のプログラムに置換する。
                *   成功すれば戻らない。
                */
               ret3 = execv(arg_c1[0], arg_c1);
               if(ret3 < 0){
                  /*
                   * execv 失敗時のみここに来る。
                   * 例:
                   *   - パスが間違っている
                   *   - 実行権限がない
                   *   - ファイルが存在しない
                   */
                  exit(0);
		       }
            }
            else{
               /*
                * 親プロセス:
                *   子の終了を待ち、ゾンビ化を防ぐ。
                */
               wait(&st);
            }
         }
         else if(ret2 == 2){
            /*
             * 2コマンドパイプの実行:
             *
             * 例:
             *   "cmd1 | cmd2"
             *
             * 実現の基本:
             *   - cmd1 の標準出力(STDOUT)をパイプの書き口へ
             *   - cmd2 の標準入力(STDIN)をパイプの読み口へ
             *
             * そのために:
             *   pipe(fd)
             *   fork で cmd1 プロセスを作り dup2(fd[1], STDOUT)
             *   fork で cmd2 プロセスを作り dup2(fd[0], STDIN)
             */

            get_arg2(divcom[0], arg_c1, " ");
            get_arg2(divcom[1], arg_c2, " ");
            /*
             * cmd1/cmd2 を空白区切りで分割して execv 用 argv 配列にする。
             */

            ret3 = pipe(fd);
            /*
             * pipe(fd):
             *   fd[0]=read end, fd[1]=write end を得る。
             *
             * パイプは1方向:
             *   write(fd[1]) → read(fd[0])
             */
            if(ret3 < 0){
               perror("pipe");
               exit(1);
            }

            pid = fork();
            if(pid == 0){
               /*
                * 子1（cmd1担当）:
                *
                * cmd1 の出力をパイプへ流すため、
                * STDOUT_FILENO を fd[1] に付け替える。
                *
                * 1) close(fd[0]) : 読み口は使わないので閉じる
                * 2) dup2(fd[1], STDOUT_FILENO):
                *      STDOUT(1) を fd[1] と同じ先（パイプ書き口）へ向ける
                * 3) execv(cmd1)
                */
               close(fd[0]);
               dup2(fd[1], STDOUT_FILENO);

               ret4 = execv(arg_c1[0], arg_c1);
               if(ret4 < 0){
                  exit(0);
               }
            }

            pid = fork();
            if(pid == 0){
               /*
                * 子2（cmd2担当）:
                *
                * cmd2 の入力をパイプから取るため、
                * STDIN_FILENO を fd[0] に付け替える。
                *
                * 1) close(fd[1]) : 書き口は使わないので閉じる
                * 2) dup2(fd[0], STDIN_FILENO):
                *      STDIN(0) を fd[0] と同じ先（パイプ読み口）へ向ける
                * 3) execv(cmd2)
                */
               close(fd[1]);
               dup2(fd[0], STDIN_FILENO);

               ret4 = execv(arg_c2[0], arg_c2);
               if(ret4 < 0){
                  exit(0);
               }
            }

            /*
             * 親プロセス:
             *   親自身はパイプを使わないので両端を閉じる。
             *
             * ここを閉じないと何が起きるか:
             *   - 親が fd[1]（書き口）を持ち続けると、
             *     cmd2 側の read は EOF を受け取れず終了できない可能性がある。
             *
             * つまり、
             *   「不要な端点は必ず閉じる」
             * がパイプ設計の鉄則になる。
             */
            close(fd[0]);
            close(fd[1]); // 2つめのwaitより前に実行しないと２つめのプロセスが終了しない

            /*
             * 子が2つあるので wait も2回必要。
             * これにより両方を回収してゾンビ化を防ぐ。
             */
            wait(&st);
            wait(&st);
         }
      }
   }

   return 0;
}

/*
 * get_arg2:
 *   strtok を使って文字列 c を sym で分割し、arg 配列にトークンを格納する。
 *
 * 引数:
 *   c   : 分割対象文字列（注意: strtok は c を破壊する）
 *   arg : トークン配列（最後は NULL になる）
 *   sym : 区切り文字列（例: "|" や " "）
 *
 * 戻り値:
 *   トークン数
 *
 * 例:
 *   c="ls -l"
 *   sym=" "
 *   → arg[0]="ls", arg[1]="-l", arg[2]=NULL, return 2
 *
 * 注意点:
 *   - strtok は連続区切りや引用符などを扱えない（簡易パーサ）
 *   - ネスト分割時は「元文字列が破壊される」点が特に重要
 */
int get_arg2(char *c, char *arg[], char *sym){
    int i = 0;

    arg[i] = strtok(c, sym);
    /*
     * 最初のトークン取得。
     * 見つからなければ NULL が返る。
     */

    while(arg[i] != NULL){
        i++;
        arg[i] = strtok(NULL, sym);
        /*
         * 2回目以降は NULL を第1引数にして、
         * 前回の続き位置からトークンを取得する。
         */
    }

    return i;
}
