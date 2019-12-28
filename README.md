# これはなに

大昔に授業で作ったスクリーンセーバー

![scrnsample](https://user-images.githubusercontent.com/33576079/71549884-818fa080-2a08-11ea-8e8c-d6c219cf5836.png)


# 実行

1. `releases` からダウンロード
2. `hack.scr /s`

---

# 準備

## `freeglut` の準備

1. Martin Payne's Windows binaries (MSVC and MinGW)
   (https://www.transmissionzero.co.uk/software/freeglut-devel/)
   から `freeglut-MinGW.zip (v3.0.0)` をダウンロード

2. ファイルを展開し、`include` フォルダと `lib` フォルダをソースコードの
   ディレクトリにコピー

3. `bin/freeglut.dll` をソースコードのディレクトリにコピー

## 言語ファイル

常に `window.txt` を読み、実行します。


# 実行

* `make` : (必要ならば)ビルドして実行
* `make demo1` : demo1.txt を実行
* `make demo2` : demo2.txt を実行
* `make demo3` : demo3.txt を実行
