import pyb, sensor, image, time, math

enable_lens_corr = False # turn on for straighter lines...
sensor.reset()
sensor.set_pixformat(sensor.RGB565) # grayscale is faster
sensor.set_framesize(sensor.QQVGA) # 120*160
sensor.skip_frames(time = 2000)

sensor.set_auto_gain(False)  # must turn this off to prevent image washout...
sensor.set_auto_whitebal(False)  # must turn this off to prevent image washout...

clock = time.clock()

uart = pyb.UART(3,9600,timeout_char=1000)
uart.init(9600,bits=8,parity = None, stop=1, timeout_char=1000)

# All lines also have `x1()`, `y1()`, `x2()`, and `y2()` methods to get their end-points
# and a `line()` method to get all the above as one 4 value tuple for `draw_line()`.

f_x = (2.8 / 3.984) * 160 # find_apriltags defaults to this if not set
f_y = (2.8 / 2.952) * 120 # find_apriltags defaults to this if not set
c_x = 160 * 0.5 # find_apriltags defaults to this if not set (the image.w * 0.5)
c_y = 120 * 0.5 # find_apriltags defaults to this if not set (the image.h * 0.5)

def degrees(radians):
    return (180 * radians) / math.pi


ch = 'L'
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
