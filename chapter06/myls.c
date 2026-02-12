#include <stdio.h>
#include <dirent.h>
/*
 * stdio.h:
 *   printf() を使用する。
 *
 * dirent.h:
 *   ディレクトリ操作用 API を使用するために必要。
 *
 *   主な関数:
 *     opendir()  : ディレクトリを開く
 *     readdir()  : ディレクトリエントリを1件ずつ取得
 *     closedir() : ディレクトリを閉じる
 *
 *   主な構造体:
 *     DIR            : ディレクトリストリーム
 *     struct dirent  : ディレクトリエントリ
 */

int main(int argc, char *argv[]) {

    DIR *dir;
    /*
     * DIR 型:
     *   ディレクトリストリームを表す構造体。
     *
     *   内部的にはファイルディスクリプタを保持し、
     *   カーネルからディレクトリ情報を順次読み取る。
     */

    struct dirent *de;
    /*
     * struct dirent:
     *   ディレクトリエントリを表す構造体。
     *
     *   主なフィールド:
     *     d_ino  : inode番号
     *     d_name : ファイル名（文字列）
     *
     *   ディレクトリとは、
     *   「ファイル名 → inode番号」の対応表である。
     */

    dir = opendir(argv[1]);
    /*
     * opendir(path):
     *   指定されたディレクトリを開く。
     *
     *   成功:
     *     DIR* を返す。
     *
     *   失敗:
     *     NULL を返す。
     *
     *   内部では open() + getdents() システムコールを
     *   利用している。
     */

    while ((de = readdir(dir)) != NULL) {
        /*
         * readdir():
         *   ディレクトリから1つのエントリを取得する。
         *
         *   成功:
         *     struct dirent* を返す。
         *
         *   全件読み終わると:
         *     NULL を返す。
         *
         *   このループで全エントリを順番に取得している。
         */

        printf("%lu %s\n", de->d_ino, de->d_name);
        /*
         * d_ino:
         *   ファイルの inode番号。
         *
         * d_name:
         *   ファイル名。
         *
         * ディレクトリは実際には
         *
         *   inode番号 + ファイル名
         *
         * の組の集合である。
         *
         * 例:
         *   12345 file.txt
         *   12346 data.bin
         */
    }

    closedir(dir);
    /*
     * closedir():
     *   ディレクトリストリームを閉じる。
     *
     *   内部のファイルディスクリプタを解放する。
     */

    return 0;
    /*
     * 正常終了。
     */
}
