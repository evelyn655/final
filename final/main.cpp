#include "mbed.h"
#include "mbed_rpc.h"
#include "bbcar.h"
#include "bbcar_rpc.h"
#include <stdlib.h>

Ticker servo_ticker;
Ticker encoder_ticker;
PwmOut pin5(D5), pin6(D6);
DigitalIn encoder(D12);
volatile int steps;
volatile int last;
BufferedSerial xbee(D10, D9);   //tx,rx
BufferedSerial pc(USBTX,USBRX); //tx,rx
BufferedSerial uart(D1,D0);     //tx,rx
DigitalOut led1(LED1);
DigitalInOut pin11(D11);

BBCar car(pin5, pin6, servo_ticker);
parallax_ping  ping1(pin11);

void RPC_Parking(Arguments *in, Reply *out);
RPCFunction rpcRPC_Parking(&RPC_Parking, "RPC_Parking");
void RPC_Line_Following(Arguments *in, Reply *out);
RPCFunction rpcRPC_Line_Following(&RPC_Line_Following, "RPC_Line_Following");
void RPC_AprilTag(Arguments *in, Reply *out);
RPCFunction rpcRPC_AprilTag(&RPC_AprilTag, "RPC_AprilTag");

void encoder_control() {
   int value = encoder;
   if (!last && value) steps++;
   last = value;
}

void RPC_Parking(Arguments *in, Reply *out)   {
    car.turn(50, -0.3);
    ThisThread::sleep_for(2500ms);
    car.stop();
    return;
}

void RPC_Line_Following(Arguments *in, Reply *out) {
    // char ch[1] = {'L'};
    // uart.write(ch, sizeof(ch));
    // printf("write ch = %c", ch[0]);
    while(1){
        printf("while loop\r\n");
        //printf("uart = %d\r\n", uart.readable());

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
            //pc.write(recv, sizeof(recv));
            printf("char= %c\r\n", recv[0]);

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

void RPC_AprilTag(Arguments *in, Reply *out)   {
    char ch[1] = {'A'};
    uart.write(ch, sizeof(ch));
    printf("write ch = %c", ch[0]);









    
    // car.turn(50, -0.3);
    // ThisThread::sleep_for(2500ms);
    // car.stop();
    return;
}

int main() {
    //pc.set_baud(9600);
    uart.set_baud(9600);
    //parallax_ping  ping1(pin11);

    char buf[256], outbuf[256];
    FILE *devin = fdopen(&xbee, "r");
    FILE *devout = fdopen(&xbee, "w");
    while (1) {
        memset(buf, 0, 256);
        for( int i = 0; ; i++ ) {
            char recv = fgetc(devin);
            if(recv == '\n') {
                printf("\r\n");
                break;
            }
            buf[i] = fputc(recv, devout);
        }
    RPC::call(buf, outbuf);
    }
}