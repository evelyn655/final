# Final Project
Combining XBee, line detection and location identification.
     
## To setup and run the program 
我的 project 共有三個子任務:
* Line following
* Turning
* Steering to apriltag     

三個子任務各寫在一個 RPC function 裡面，     
透過 XBee 發送 RPC 指令，可以讓 BBcar 開始執行該子任務。
* 溝通介面：
    * 使用者、Mbed板：XBee
    * Mbed板、openMV：uart 介面

### [ Mbed - main.cpp ]
讀取 control.py 透過 XBee 發送的 RPC 指令，並跳至對應的 RPC function 執行該任務。
#### Line following:
```
void RPC_Line_Following(Arguments *in, Reply *out) {
    while(1){
        printf("while loop\r\n");
        printf("%lf cm \r\n",(float)ping1);
        if((float)ping1>30) led1 = 1;
        else {
            led1 = 0;
            car.stop();
            break;
        }
        if(uart.readable()){
            char recv[1];
            uart.read(recv, sizeof(recv));  // &recv[0]         
            if (recv[0] == '1') {   // see line, go straight
                printf("straight\r\n");
                car.goStraight(50);
                ThisThread::sleep_for(500ms);
                car.stop();
            } else if (recv[0] == '0') {   // stop
                printf("stop\r\n");
                car.stop();
                ThisThread::sleep_for(500ms);
            } 
            ThisThread::sleep_for(500ms);
      }
   }
}
```

1. 讀取 openMV 透過 uart 介面回傳的資訊（若有看到 line，傳入 1，若沒有則傳入 0），決定 BBcar 是否往前直走。     
3. 每 1 秒讀取一次數值（所以 openMV 端也配合每 1 秒才寫入一次數值），並決定 BBcar 的運動情形（go straight 或 stop）。          
4. 使用 ping 測量與前方障礙物的距離，若小於 30 公分則停下，break 迴圈，line following 任務結束。          
     
            
#### Turning:
單純讓 BBcar 向右轉一個角度，使 BBcar 偏離前方障礙物，並轉往 apriltag 的方向。
```
void RPC_Parking(Arguments *in, Reply *out)   {
    car.turn(50, -0.3);
    ThisThread::sleep_for(2500ms);
    car.stop();
    return;
}
```
              
               
#### Steering to apriltag:
```
void RPC_AprilTag(Arguments *in, Reply *out)   {
    char ch[1] = {'A'};
    uart.write(ch, sizeof(ch));
    printf("write ch = %c", ch[0]);

    while(1){
        printf("*********while loop!********\r\n");
        printf("%lf cm \r\n",(float)ping1);
        if((float)ping1>30) led1 = 1;
        else {
            led1 = 0;
            car.stop();
            break;
        }

        if(uart.readable()){

            char recv[1];
            uart.read(recv, sizeof(recv));  // &recv[0]
            
            printf("char= %c\r\n", recv[0]);
            
            if (recv[0] == 's') {
                car.goStraight(40);
                ThisThread::sleep_for(1500ms);
                car.stop();
            } else if (recv[0] == 'r') {   // turn right
                car.turn(40, -0.85);
                ThisThread::sleep_for(1500ms);
                car.stop();
            } else if (recv[0] == 'l') {   // turn left
                car.turn(40, 0.9);
                ThisThread::sleep_for(1500ms);
                car.stop();
            } 
            ThisThread::sleep_for(500ms);
      }
   }
    return;
}
```
1. 透過 uart 介面將一字母 'A' 傳送給 openMV，讓 openMV 知道該切換成找 apriltag 的模式。       
2. * 若收到 openMV 傳來的 's'，則 BBcar 往前走 1.5s         
    * 若收到 'r'（代表 BBcar 現在偏右），則讓 BBcar 很小幅度的右轉 1.5s，慢慢修正，左邊同理。        
    * 每2秒 read 一次數值（所以 main.py 中也配合每 2s 才 write 一次數值），一小段一小段前進、修正，直到 BBcar 慢慢轉正、面對 apriltag，接著就會直線前進。               
3. 使用 ping 測量與前方障礙物的距離，若小於 30 公分則停下，break 迴圈，steering to apriltag 任務結束。          
     
          
### [ openMV - main.py ]
```
num = 0
while(True):
    clock.tick()
    img = sensor.snapshot()
    if enable_lens_corr: img.lens_corr(1.8) # for 2.8mm lens...

    if uart.any():
        ch = uart.read()
        print("read")
        #print(uart.any())
        num = 1
    print(num)


    if num == 0:
        see_line = 0
        for l in img.find_line_segments(merge_distance = 0, max_theta_diff = 5):
            if l.magnitude() > 12:
                if l.y2() < 50 and abs(l.y2()-l.y1()>10) :
                    if l.x1()>20 and l.x2()<100:
                        img.draw_line(l.line(), color = (255, 0, 0))
                        print(l)
                        see_line = 1
                        #uart.write(("L").encode())
                        #time.sleep(0.5)
        uart.write(("%d" % see_line).encode()) # 1: see line, 0: no line
        time.sleep(1)
        print("see_line = %d" % see_line)
        print("FPS %f" % clock.fps())
    elif num ==  1:
        print("Apriltag mode")
        for tag in img.find_apriltags(fx=f_x, fy=f_y, cx=c_x, cy=c_y): # defaults to TAG36H11
            img.draw_rectangle(tag.rect(), color = (255, 0, 0))
            img.draw_cross(tag.cx(), tag.cy(), color = (0, 255, 0))
            print("Ry %f" % degrees(tag.y_rotation()))
            if degrees(tag.y_rotation()) <= 5 or (degrees(tag.y_rotation()) >=355 and degrees(tag.y_rotation())<=360):
                uart.write(("s").encode())      # go straight
                time.sleep(2);
                #uart.write(("  Ry: %f\r\n" % degrees(tag.y_rotation())).encode())
            elif (degrees(tag.y_rotation()) >=180 and degrees(tag.y_rotation())<355):
                uart.write(("r").encode())      # slightly turn left (the car seen from the right)
                time.sleep(2);
                #uart.write(("  Ry: %f\r\n" % degrees(tag.y_rotation())).encode())
            elif (degrees(tag.y_rotation()) >5 and degrees(tag.y_rotation())<180):
                uart.write(("l").encode())      # slightly turn right (the car seen from the left)
                time.sleep(2);
                #uart.write(("  Ry: %f\r\n" % degrees(tag.y_rotation())).encode())
```                  
1. 預設 num = 0，line following 任務模式。         
    * 透過 magnitude、x1、x2、y1、y2 等數值，可以乾淨的篩選出畫面出希望 BBcar 跟著前進的那條線。        
    * 根據畫面中有沒有看到符合條件的線，將 see_line 的值 write 到 uart 介面，在 mbed 端可以根據 read 到的值決定 BBcar 的運動模式。           
    * 配合 Mbed 端 read 的速度，這裡每 1 秒才寫入一次資訊。        
    
2. 若有收到 Mbed 端透過 uart 寫入的資訊，則使 num = 1，切換成找 apriltag 的模式。     
    * 依據 tag.y_roataion() 的值將 apriltag 和 BBcar 的相對位置分成三類：        
        * 大約在正前方（355°-5°） - write 's' 到 uart 介面          
        * 在鏡頭畫面中偏右（355°-180°） - write 's' 到 uart 介面        
        * 在鏡頭畫面中偏左（5°-180°） - write 'l' 到 uart 介面       
    * 配合 Mbed 端 read 的速度，這裡每 2 秒才寫入一次資訊。         

### 透過 XBee 發送 RPC 指令 - control.py 
使用者的角色。        
這裡分別間隔 25s、10s 後發送下一個子任務的 RPC 指令，透過 XBee 和 BBcar 溝通。         
```
print("Sending /RPC_Line_Following/run\n")
s.write("/RPC_Line_Following/run\n".encode())
time.sleep(25)
print("Sending /RPC_Parking/run\n")
s.write("/RPC_Parking/run\n".encode())
time.sleep(10)
print("/RPC_AprilTag/run\n")
s.write("/RPC_AprilTag/run\n".encode())
```

## The results
https://drive.google.com/file/d/1sneGROKyU0rfZCc18icyvlvaZKxLSwo7/view?usp=sharing
