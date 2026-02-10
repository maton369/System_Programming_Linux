#include <stdio.h>
/*
 * stdio.h:
 *   標準入出力ライブラリ。
 *   scanf(), fprintf(), stderr などの宣言が含まれる。
 *
 * scanf は「入力先のアドレス」を必要とする関数であり、
 * ポインタや配列・構造体との関係を理解することが重要。
 */

/*
 * struct denwacho:
 *   電話帳の1件分を表す構造体。
 *
 * 構造体とは:
 *   複数の異なる型のデータをひとまとめにし、
 *   1つの「まとまり」として扱うための仕組み。
 *
 * メンバはメモリ上に宣言順で連続して配置される
 * （※アラインメントによる隙間が入る場合あり）。
 */
struct denwacho{
   char name[40];
   /*
    * name:
    *   文字配列（char の配列）。
    *   最大 39 文字 + 終端文字 '\0' を想定。
    *
    * 配列名 name は式中では
    *   「&name[0]（先頭要素のアドレス）」
    * として扱われる。
    */

   char tel[12];
   /*
    * tel:
    *   電話番号を格納するための文字配列。
    *   こちらも文字列として扱うため、
    *   実体は「連続した char の並び」である。
    */

   int age;
   /*
    * age:
    *   年齢を表す整数値。
    *   char 配列とは異なり、
    *   int は数値そのものを格納する。
    */
};

int main(){
    struct denwacho friend;
    /*
     * friend:
     *   struct denwacho 型の変数。
     *
     * main 関数内で宣言されているため、
     * 構造体全体はスタック領域に確保される。
     *
     * メモリ上の概念図:
     *
     *   friend
     *   ├─ name[0] ... name[39]
     *   ├─ tel[0]  ... tel[11]
     *   └─ age
     */

    scanf("%s", friend.name);
    /*
     * %s:
     *   文字列（空白まで）を読み込む指定子。
     *
     * friend.name:
     *   char 配列であり、
     *   関数呼び出し時には
     *   「先頭要素のアドレス (&friend.name[0])」
     * が渡される。
     *
     * そのため & は不要。
     *
     * 注意:
     *   入力文字数の上限チェックがないため、
     *   長い入力を与えるとバッファオーバーフローの危険がある。
     */

    scanf("%s", friend.tel);
    /*
     * tel も name と同様に char 配列。
     *
     * 配列名はポインタとして評価されるため、
     * scanf にそのまま渡すことができる。
     */

    scanf("%d", &friend.age);
    /*
     * %d:
     *   int 型の入力を読み込む指定子。
     *
     * friend.age:
     *   int 型の「値」そのもの。
     *
     * scanf は「書き込み先のアドレス」を必要とするため、
     * &friend.age として
     * age のメモリアドレスを渡している。
     */

    fprintf(stderr, "friend.name=%s\n", friend.name);
    /*
     * name は文字列なので %s で出力。
     * 配列名は先頭アドレスとして扱われる。
     */

    fprintf(stderr, "friend.tel=%s\n", friend.tel);
    /*
     * tel も同様に文字列として出力。
     */

    fprintf(stderr, "friend.age=%d\n", friend.age);
    /*
     * age は int 型の値そのものなので %d で出力。
     * 出力時はアドレスではなく値を渡す点に注意。
     */

    return 0;
    /*
     * プログラムの正常終了。
     * 戻り値 0 は OS に対して
     * 「正常終了」を示す。
     */
}
