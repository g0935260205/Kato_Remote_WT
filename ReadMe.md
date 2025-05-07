# 功能表
## 主要目標
1. 無線控制車輛
2. 連線至WiThrottle Server
3. 透過網頁設定
   1. WiFi
   2. JMRI/DCC-EX IP/Port
   3. 車號
4. 可自動進入睡眠模式
5. 連線中斷時嘗試連線
6. 初始化時等待連線完成
7. 
## 功能
1. mDNS搜尋 WiThrottle Server
2. 快速進入睡眠
   1. stop位，速度設為紅線以上 10秒後進入睡眠模式
3. 進入設定模式，重新啟動時可進入
   1. WiFi未尋找到時
   2. 速度設為紅線以下，中速以上，撥動方向桿，即可進入

## 設定值
### 必須
1. WiFi SSID
2. WiFi PSWD 
3. ADDR: S1-S127/L128-L9999

## 選擇性
1. WT_IP:192.168.4.1
2. WT_Port:
   1. JMRI:12090
   2. DCCex:2560