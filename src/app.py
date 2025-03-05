import serial # 序列埠通訊
import time # 時間處理
import cv2  # OpenCV 影像處理
import numpy as np # 數值計算
from PIL import Image # 影像處理
import io # 輸入輸出
import tensorflow as tf  # 直接匯入 tensorflow
from tensorflow import keras
from keras.models import load_model 
from keras.utils import img_to_array

class WasteClassifier:
    def __init__(self, port='COM3', baud_rate=115200):
        self.serial_port = serial.Serial(port, baud_rate, timeout=1)
        time.sleep(2)  # 等待 2 秒讓串口初始化完
        # 新增: 載入訓練好的模型
        self.model = load_model('src/keras_model.h5')
        # 新增: 類別標籤
        self.labels = ['plastic', 'paper', 'Aluminium']
        
    def capture_image(self):
        # 向 ESP32-CAM 發送拍照命令
        self.serial_port.write(b'capture\n')
        
        # 等待並讀取影像大小
        size_line = self.serial_port.readline().decode().strip()
        if not size_line.startswith('SIZE:'):
            return None
        
        image_size = int(size_line.split(':')[1])
        
        # 讀取影像資料
        data_line = self.serial_port.readline()
        if not data_line.startswith(b'DATA:'):
            return None
            
        # data_line[5:] 跳過 'DATA:' 標頭
        # 讀取剩餘的影像資料
        image_data = data_line[5:] + self.serial_port.read(image_size - len(data_line) + 5)
        
        # 將二進制資料轉換為影像
        try:
            # 使用 BytesIO 創建記憶體中的檔案物件
            # 使用 PIL 讀取 JPEG 影像
            image = Image.open(io.BytesIO(image_data))
            # 將 PIL 影像轉換為 OpenCV 格式（BGR）
            return cv2.cvtColor(np.array(image), cv2.COLOR_RGB2BGR)
        except Exception as e:
            print(f"Error converting image: {e}")
            return None
    
    def predict_image(self, image):
        # 影像預處理
        img = cv2.resize(image, (224, 224))  # 調整為模型輸入大小
        img = img_to_array(img)
        img = np.expand_dims(img, axis=0)
        img = img / 255.0  # 正規化
        
        # 預測
        predictions = self.model.run(None, {'input_1': img})
        predicted_class = self.labels[np.argmax(predictions[0][0])]
        confidence = np.max(predictions[0][0])
        
        return predicted_class, confidence
    
    def control_gate(self, category):
        # 檢查分類類別是否有效
        if category in ['plastic', 'paper', 'Aluminium']:
            # 發送控制命令到 ESP32-CAM
            self.serial_port.write(f'{category}\n'.encode())
            # 讀取回應
            response = self.serial_port.readline().decode().strip()
            return response
        return "Invalid category"
    
    def close(self):
        # 關閉串口連接
        self.serial_port.close()

def main():
    # 創建分類器實例
    classifier = WasteClassifier()
    
    try:
        while True:
            # 拍照
            image = classifier.capture_image()
            if image is not None:
                # 顯示影像
                cv2.imshow('Waste Classification', image)
                
                # 新增: 自動分類
                predicted_class, confidence = classifier.predict_image(image)
                
                # 如果信心度超過閾值，才進行分類
                if confidence > 0.7:  # 70% 信心度
                    print(f"預測類別: {predicted_class}, 信心度: {confidence:.2f}")
                    print(classifier.control_gate(predicted_class))
                
                # 保留手動控制用於測試
                key = cv2.waitKey(1) & 0xFF
                if key == ord('q'):
                    break
                    
    finally:
        classifier.close()
        cv2.destroyAllWindows()
        
# 程式入口點
if __name__ == '__main__':
    main() 