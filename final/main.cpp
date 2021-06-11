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

void RPC_Parking(Arguments *in, Reply *out);
RPCFunction rpcRPC_Parking(&RPC_Parking, "RPC_Parking");
void RPC_Line_Following(Arguments *in, Reply *out);
RPCFunction rpcRPC_Line_Following(&RPC_Line_Following, "RPC_Line_Following");

void encoder_control() {
   int value = encoder;
   if (!last && value) steps++;
   last = value;
}

void RPC_Parking(Arguments *in, Reply *out)   {
    double d1 = in->getArg<double>();
    double d2 = in->getArg<double>();
    char face = in->getArg<char>();

    if (face=='S') {
        // go back d2 cm
        encoder_ticker.attach(&encoder_control, 10ms);
        steps = 0;
        last = 0;
        car.goStraight(-50);

        printf("d1 = %f\n", d1);
        printf("d2 = %f\n", d2);
        printf("face = %c\n", face);

        while(steps*6.5*3.14/32 < (d2-5+7)) {
            printf("encoder = %d\r\n", steps);
            ThisThread::sleep_for(100ms);
        }
        car.stop();
    } else {
        // go back d1 cm
        encoder_ticker.attach(&encoder_control, 10ms);
        steps = 0;
        last = 0;
        car.goStraight(-50);

        printf("d1 = %f\n", d1);
        printf("d2 = %f\n", d2);
        printf("face = %c\n", face);

        while(steps*6.5*3.14/32 < (d1-5+17)) {
            printf("encoder = %d\r\n", steps);
            ThisThread::sleep_for(100ms);
        }
        car.stop();
        ThisThread::sleep_for(3000ms);

        
        // turning 
        if (face=='U') {
            car.turn(50, 0.3);
            ThisThread::sleep_for(2500ms);
            car.stop();
        } else if (face=='D') {
            car.turn(50, -0.3);
            ThisThread::sleep_for(2500ms);
            car.stop();
        }
        ThisThread::sleep_for(3000ms);
        
        // go back d2 cm
        encoder_ticker.attach(&encoder_control, 10ms);
        steps = 0;
        last = 0;
        car.goStraight(-50);

        printf("d1 = %f\n", d1);
        printf("d2 = %f\n", d2);
        printf("face = %c\n", face);

        while(steps*6.5*3.14/32 < (d2-5+15.5+11)) {
            printf("encoder = %d\r\n", steps);
            ThisThread::sleep_for(100ms);
        }
        car.stop();

    }
    return;
}

void RPC_Line_Following(Arguments *in, Reply *out) {
    while(1){
        printf("while loop\r\n");
        printf("uart = %d\r\n", uart.readable());
        if(uart.readable()){
            char recv[1];
            uart.read(recv, sizeof(recv));  // &recv[0]
            //pc.write(recv, sizeof(recv));
            //output = atoi(recv);
            printf("char= %c\r\n", recv[0]);

            if (recv[0] == '1') {   // see line, go straight
                printf("straight\r\n");
                car.goStraight(50);
                ThisThread::sleep_for(500ms);
                car.stop();
                
                
                // ping detection to stop the car
                // or at the end, break the while loop?

            } else if (recv[0] == '0') {   // stop
                printf("stop\r\n");
                car.stop();
                ThisThread::sleep_for(500ms);
            } 
            ThisThread::sleep_for(500ms);
      }
   }
}



int main() {
    //pc.set_baud(9600);
    uart.set_baud(9600);
    parallax_ping  ping1(pin11);

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