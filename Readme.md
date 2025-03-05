# SmartBean 智慧垃圾分類系統

## 專案簡介
SmartBean 是一個基於 ESP32-CAM 的智慧垃圾分類系統，使用機器學習模型自動識別並分類垃圾種類。

## 功能特點
- 即時影像擷取
- AI 垃圾分類（支援塑膠、紙類、鋁製品）
- 自動分類閘門控制
- 即時顯示分類結果

## 環境需求
### 硬體需求
- ESP32-CAM 開發板
- 伺服馬達
- USB-TTL 轉換器

### 軟體需求
- Python 3.10
- PlatformIO IDE
- Visual Studio Code

## 安裝步驟
1. 安裝 Python 環境：

# 安裝依賴套件
pip install -r requirements.txt
```

2. 安裝 PlatformIO IDE：
- 在 VS Code 中安裝 PlatformIO IDE 擴充功能
- 開啟專案資料夾
- PlatformIO 會自動安裝必要的開發套件

## 使用說明
1. 連接硬體：
   - 將 ESP32-CAM 連接到電腦
   - 確認序列埠設定

2. 上傳程式：
   - 使用 PlatformIO 上傳韌體到 ESP32-CAM
   - 等待上傳完成

3. 執行程式：
```bash
python src/app.py
```

## 專案結構
```
SmartBean/
├── src/                # 源碼目錄
│   ├── app.py         # Python 主程式
│   ├── main.cpp       # ESP32 韌體
│   └── keras_model.h5 # AI 模型
├── requirements.txt    # Python 依賴
└── platformio.ini     # PlatformIO 設定
```

## 常見問題
1. 序列埠連接問題
   - 檢查 COM 埠設定
   - 確認驅動程式安裝

2. 模型載入錯誤
   - 確認模型檔案位置
   - 檢查 tensorflow 版本



