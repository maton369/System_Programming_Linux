#include <stdio.h>
/*
 * stdio.h:
 *   標準入出力ライブラリ。
 *   fprintf(), stderr などの宣言が含まれる。
 *
 * stderr は標準エラー出力であり、
 * デバッグ情報の表示によく使われる。
 */

int main(int argc, char *argv[]){
    /*
     * argc (argument count):
     *   コマンドライン引数の個数を表す整数。
     *
     *   実行ファイル名自身も 1 個の引数として数えられるため、
     *   引数を何も付けずに実行した場合でも argc は 1 になる。
     *
     * 例:
     *   ./a.out
     *     → argc = 1
     *
     *   ./a.out foo bar
     *     → argc = 3
     */

    /*
     * argv (argument vector):
     *   コマンドライン引数を格納した「文字列配列」。
     *
     *   型は char *argv[] だが、
     *   実体は
     *     char **argv
     *   と等価であり、
     *   「char へのポインタの配列」を意味する。
     *
     *   各 argv[i] は、
     *   ヌル終端された文字列（C文字列）へのポインタ。
     */

    fprintf(stderr, "argc=%d\n", argc);
    /*
     * argc の値を表示。
     * 実行時に与えられた引数の総数を確認できる。
     */

    fprintf(stderr, "argv[0]=%s\n", argv[0]);
    /*
     * argv[0]:
     *   実行されたプログラム名（またはそのパス）。
     *
     *   OS がプロセス生成時に自動的に設定する。
     *   ユーザーが明示的に指定した引数ではない点に注意。
     */

    fprintf(stderr, "argv[1]=%s\n", argv[1]);
    /*
     * argv[1]:
     *   最初にユーザーが指定した引数。
     *
     *   ただし、argc が 2 未満の場合、
     *   argv[1] は存在せず、
     *   この参照は未定義動作になる。
     */

    fprintf(stderr, "argv[2]=%s\n", argv[2]);
    /*
     * argv[2]:
     *   2 番目のユーザー指定引数。
     *
     *   argc < 3 の場合は存在しないため、
     *   実務では必ず
     *     if (argc > 2)
     *   のようなチェックが必要。
     */

    return 0;
    /*
     * プログラムの正常終了。
     * 戻り値 0 は OS に対して
     * 「正常終了」を意味する。
     */
}
