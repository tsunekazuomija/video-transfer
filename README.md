# 片方向画像送信プログラム

サーバーからクライアントにウェブカメラで撮影した写真を送信する。
逆方向、双方向には対応していない。

ubuntuで1台のコンピュータでサーバ、クライアント両方を実行する場合の動作確認済み。
複数台での通信は未検証


## インストール
- fswebcam
  - ubuntuでウェブカメラを使用するために使う
- gtk+
  - c言語でGUIを作成するために使う
  - クライアント側で画像を1つのウィンドウに表示するのに使ってる
  - これがないと画像を表示するたびに新しいウィンドウが作られてしまう

fswebcamのインストール
```
sudo apt-get install fswebcam
```

gtk+のインストール
```
sudo apt-get install libgtk-3-dev
```

## コンパイル

サーバは以下のようにコンパイルする  
```
gcc -o video_server video_server.c
```

クライアントは以下のようにコンパイルする  
```
gcc -o video_client video_client.c `pkg-config --cflags --libs gtk+-3.0`
```

## 実行

サーバ側を実行する
```
./video_server <port>
```

クライアント側を実行する
```
./video_client <server_ip> <server_port>
```

終わるときはクライアント側でCtrl+Cを押す、またはウィンドウを閉じるとサーバ側も終了する。

## 注意

サーバ側、クライアント側ともに自分と同じ階層にあるtmpディレクトリに画像を一時保存しているので、
前もってtmpディレクトリを作っておく必要がある。

ディレクトリ構成
```
.
├── README.md
├── video_client
│   ├── tmp
│   ├── video_client
│   └── video_client.c
└── video_server
    ├── tmp
    ├── video_server
    └── video_server.c
```