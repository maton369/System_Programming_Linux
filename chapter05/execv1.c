#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
/*
 * stdio.h:
 *   本コードでは直接使用していないが、
 *   一般に標準入出力関数を利用する場合に必要。
 *
 * stdlib.h:
 *   exit() などの関数を使用する際に必要。
 *
 * unistd.h:
 *   execv() を使用するために必要。
 *
 * exec 系システムコールは、
 * 「現在のプロセスを別プログラムに置き換える」
 * ための API である。
 */

int main(){

    char command[256], *argv[32];
    /*
     * command:
     *   本コードでは未使用。
     *   通常はユーザー入力を格納する用途で使う。
     *
     * argv:
     *   execv() に渡す引数配列。
     *
     *   execv の第2引数は
     *   「char *argv[]」形式の配列である。
     *
     *   ルール:
     *     argv[0] = プログラム名
     *     argv[1..n] = 引数
     *     最後は NULL
     */

    argv[0] = "/bin/ls";
    /*
     * argv[0] は実行するプログラム名。
     *
     * execv では第1引数と argv[0] を
     * 通常同じ値にする。
     *
     * ここでは /bin/ls を実行する。
     */

    argv[1] = NULL;
    /*
     * 引数の終端を示す NULL。
     *
     * execv は NULL 終端を必須とする。
     * これがないと未定義動作になる。
     */

    execv(argv[0], argv);
    /*
     * execv():
     *   現在のプロセスを、
     *   argv[0] で指定されたプログラムで完全に置き換える。
     *
     * 引数:
     *   第1引数: 実行ファイルのパス
     *   第2引数: argv 配列
     *
     * 実行される側（/bin/ls）から見ると:
     *
     *   argv[0] = "/bin/ls"
     *   argv[1] = NULL
     *
     * つまり、引数なしの ls が実行される。
     *
     * 【重要】
     *   execv が成功した場合、
     *   この関数は戻らない。
     *
     *   - 現在のプログラムコード
     *   - スタック
     *   - ヒープ
     *   - データ領域
     *
     *   すべてが /bin/ls のものに置き換わる。
     *
     *   PID は変わらない。
     */

    // execl(argv[0],argv[0],NULL);
    /*
     * 参考:
     *   execl は可変長引数版の exec。
     *
     *   execv:
     *       配列で渡す
     *
     *   execl:
     *       引数を列挙して渡す
     */

    return 0;
    /*
     * 通常ここには到達しない。
     *
     * execv が成功すれば、
     * main の残りのコードは実行されない。
     *
     * ここに来る場合は execv が失敗している。
     */
}
