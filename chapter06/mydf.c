#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/vfs.h>
/*
 * stdio.h:
 *   fprintf() を使用するために必要。
 *
 * unistd.h:
 *   sync() を使用するために必要。
 *
 * stdlib.h:
 *   exit() を使用するために必要。
 *
 * sys/vfs.h:
 *   statfs() と struct statfs を使用するために必要。
 *
 * statfs() は、指定されたパスが属する
 * ファイルシステムの情報を取得するシステムコールである。
 */

int main(int argc, char *argv[]){

   struct statfs buf;
   /*
    * struct statfs:
    *   ファイルシステムの情報を格納する構造体。
    *
    * 主なフィールド:
    *   f_blocks  : 総ブロック数
    *   f_bfree   : 空きブロック数
    *   f_bsize   : ブロックサイズ
    */

   double gb = 1024.0 * 1024.0 * 1024.0;
   /*
    * 1GB をバイト単位で定義。
    * 表示を GB 単位に変換するために使用。
    */

   int ubs, ret;
   /*
    * ubs:
    *   used block size（使用ブロック数）を保持する変数。
    *
    * ret:
    *   statfs() の戻り値。
    */

   sync();
   /*
    * sync():
    *   ディスクへの書き込みを強制的に同期させる。
    *
    * カーネルのバッファキャッシュにあるデータを
    * ディスクに書き込ませる。
    *
    * これにより、より正確なディスク使用量を取得できる。
    */

   ret = statfs(argv[1], &buf);
   /*
    * statfs(path, &buf)
    *
    * 第1引数:
    *   情報を取得したいパス（例: "/", "/home" など）
    *
    * 第2引数:
    *   結果を書き込む構造体。
    *
    * 成功:
    *   0 を返す。
    *
    * 失敗:
    *   -1 を返す。
    *
    * この関数はカーネルに問い合わせを行い、
    * 指定パスが属するファイルシステムの統計情報を取得する。
    */

   if(ret < 0){
      /*
       * エラー処理。
       * 本来は perror() などで原因を表示すべき。
       */
      exit(0);
   }

   fprintf(stderr,"%.1f GB\n", buf.f_blocks * buf.f_bsize / gb);
   /*
    * 総容量の計算。
    *
    * f_blocks:
    *   ファイルシステム全体のブロック数。
    *
    * f_bsize:
    *   1ブロックあたりのサイズ（バイト）。
    *
    * 総容量（バイト） =
    *     f_blocks × f_bsize
    *
    * これを GB に変換して表示。
    */

   ubs = buf.f_blocks - buf.f_bfree;
   /*
    * 使用中ブロック数の計算。
    *
    * f_blocks  : 総ブロック数
    * f_bfree   : 空きブロック数
    *
    * 使用ブロック数 =
    *     f_blocks - f_bfree
    */

   fprintf(stderr,"usedsize=%.0f GB\n", ubs * buf.f_bsize / gb);
   /*
    * 使用容量の表示。
    *
    * 使用容量（バイト） =
    *     使用ブロック数 × ブロックサイズ
    */

   fprintf(stderr,"freesize=%.0f GB\n",
           buf.f_bfree * buf.f_bsize / gb);
   /*
    * 空き容量の表示。
    *
    * 空き容量（バイト） =
    *     空きブロック数 × ブロックサイズ
    */

   fprintf(stderr,"used rasio=%.0f %%\n",
           100.0 * ubs / buf.f_blocks);
   /*
    * 使用率の計算。
    *
    * 使用率 (%) =
    *     (使用ブロック数 / 総ブロック数) × 100
    *
    * rasio は ratio の誤記。
    */

   return 0;
   /*
    * 正常終了。
    */
}
