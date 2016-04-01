//  PIC32 Server - Microchip BSD stack socket API
// MPLAB X C32 Compiler     PIC32MX795F512L
//      Microchip DM320004 Ethernet Starter Board

//     ECE4532    Dennis Silage, PhD
// main.c
// version 1-27-14

//      Starter Board Resources:
//    LED1 (RED) RD0
//    LED2 (YELLOW)  RD1
//    LED3 (GREEN)   RD2
//    SW1       RD6
//    SW2       RD7
//    SW3       RD13

// Control Messages
//    02 start of message
//    03 end of message


#include <string.h>

#include <plib.h>     // PIC32 Peripheral library functions and macros
#include "tcpip_bsd_config.h"  // in \source
#include <TCPIP-BSD\tcpip_bsd.h>

#include "hardware_profile.h"
#include "system_services.h"
#include "display_services.h"

#include "mstimer.h"
#include <math.h>

#define PC_SERVER_IP_ADDR "192.168.2.105"  // check ipconfig for IP address
#define SYS_FREQ (80000000)

void DelayMsec(unsigned int);

int main()
{

            
            int rlen;
            int d;
            int flag;
            int tlen = 42;
            int transfer = 0;
            char rcbfr[42];
            char rtbfr[42];
            //int tlen1 = 1;
            //int         dly1 = 10000, dly2 = 6000;
            SOCKET        srvr, StreamSock = INVALID_SOCKET;
            IP_ADDR       curr_ip;
            static BYTE    rbfr[50];     // receive data buffer
            static BYTE tbfr[1500];    // transmit data buffer
            struct        sockaddr_in addr;
            int addrlen =  sizeof(struct sockaddr_in);
            unsigned int   sys_clk, pb_clk;

// LED setup
            mPORTDSetPinsDigitalOut(BIT_0 | BIT_1 | BIT_2 );   // RD0, RD1 and RD2 as outputs
            mPORTDClearBits(BIT_0 | BIT_1 | BIT_2);
// switch setup
            mPORTDSetPinsDigitalIn(BIT_6 | BIT_7 | BIT_13);     // RD6, RD7, RD13 as inputs

// system clock
            sys_clk=GetSystemClock();
            pb_clk=SYSTEMConfigWaitStatesAndPB(sys_clk);
// interrupts enabled
            INTEnableSystemMultiVectoredInt();
// system clock enabled
            SystemTickInit(sys_clk, TICKS_PER_SECOND);

// initialize TCP/IP
            TCPIPSetDefaultAddr(DEFAULT_IP_ADDR, DEFAULT_IP_MASK, DEFAULT_IP_GATEWAY,
                    DEFAULT_MAC_ADDR);
            if (!TCPIPInit(sys_clk))
                return -1;
            DHCPInit();

// create TCP server socket
            if((srvr = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP )) == SOCKET_ERROR )
                return -1;
// bind to a local port
            addr.sin_port = 6653;
            addr.sin_addr.S_un.S_addr = IP_ADDR_ANY;
            if( bind(srvr, (struct sockaddr*)&addr, addrlen ) == SOCKET_ERROR )
                return -1;
            listen(srvr, 5 );


            while(1)
                {
                IP_ADDR ip;

                TCPIPProcess();
                DHCPTask();

                ip.Val = TCPIPGetIPAddr();
                if(curr_ip.Val != ip.Val)     // DHCP server change IP address?
                curr_ip.Val = ip.Val;

// TCP Server Code
                if(StreamSock == INVALID_SOCKET)
                    {
                    StreamSock = accept(srvr, (struct sockaddr*)&addr, &addrlen );
                    if(StreamSock != INVALID_SOCKET)
                        {
                        setsockopt(StreamSock, SOL_SOCKET, TCP_NODELAY, (char*)&tlen, sizeof(int)); // SO_SNDBUF
                        mPORTDSetBits(BIT_0);   // LED1=1
                        DelayMsec(50);
                        mPORTDClearBits(BIT_0); // LED1=0
                        mPORTDSetBits(BIT_1);   // LED2=1
                        DelayMsec(50);
                        mPORTDClearBits(BIT_1); // LED2=0
                        mPORTDSetBits(BIT_2);   // LED3=1
                        DelayMsec(50);
                        mPORTDClearBits(BIT_2); // LED3=0
                        }
                    }
                else
                        {
// receive TCP data
                        rlen = recvfrom(StreamSock, rbfr, sizeof(rbfr), 0, NULL, NULL);
                        if(rlen > 0)
                            {
                            if (rbfr[0]==2)    // 02 start of message
//                                mPORTDSetBits(BIT_0);    // LED1=1
                                {
                                if(rbfr[1]==71)    //G global reset
                                    {
                                    mPORTDSetBits(BIT_0);   // LED1=1
                                    DelayMsec(50);
                                    mPORTDClearBits(BIT_0); // LED1=0
                                    }
                                }
                            
                            if(rbfr[1]==84)    //T transfer
                                {
                                transfer++;
                                char finalCode[42];

                                if ((transfer % 2) == 1)
                                {
                                    flag = 1;
                                    // Entire message
                                    char message[] = "EE is my avocation";
                                    // Generator matrix
                                    int G[3][6] = {{1,0,0,1,1,0},
                                                   {0,1,0,1,1,1},
                                                   {0,0,1,1,0,1}};

                                    int ind;
                                    int i;
                                    int y;
                                    // Entire message (18 ASCII characters)
                                    int mcounter = 0;
                                    int fcounter = 0;
                                    for (y = 1; y< 7; y++)
                                    {
                                        int total_binary[21] = {0,0,0,0,0,0,0,
                                                            0,0,0,0,0,0,0,
                                                            0,0,0,0,0,0,0};
                                        int counter=0;
                                        // 7 3-bit packets (3 ASCII characters)
                                        for (ind = 0; ind < 3; ind++)
                                        {
                                            for (ind = 0; ind < 3; ind++)
                                            {
                                                int binary_value[7] = {0,0,0,0,0,0,0};
                                                char c = message[mcounter];
                                                mcounter++;
                                                // convert ASCII to binary
                                                for (i = 0; i < 7; ++i) {
                                                    binary_value[6-i] = (c >> i) & 1;
                                                }

                                                for (i = 0; i < 7; ++i) {
                                                    total_binary[counter] = binary_value[i];
                                                    counter++;
                                                }        
                                            }
                                        }

                                        int row;
                                        int col;
                                        int temp;
                                        int sthree;
                                        int gcounter = 0;

                                        // Multiply each 3-bit packet with generator matrix
                                        for (sthree = 0; sthree < 7; sthree++)
                                        {
                                            int codeWord[6] = {0,0,0,0,0,0};
                                            for (col = 0; col < 6; col++)
                                            {
                                                temp = 0;
                                                for (row = 0; row < 3; row++)
                                                {
                                                    temp = total_binary[row+gcounter]*G[row][col] + temp;
                                                }
                                                codeWord[col] = temp;
                                            }
                                            gcounter = gcounter + 3;
                                            int w;
                                            for (w = 0; w < 6; w++)
                                            {
                                                if ((codeWord[w] % 2)  == 0)
                                                {
                                                    codeWord[w] = 0;
                                                }
                                                if ((codeWord[w] % 2)  == 1)
                                                {
                                                    codeWord[w] = 1;
                                                }
                                            }
                                            int dec = 32*codeWord[0] + 16*codeWord[1] + 8*codeWord[2] + 4*codeWord[3] + 2*codeWord[4] + 1*codeWord[5];
                                            finalCode[fcounter] = dec;
                                            fcounter++;
                                        }
                                    }

                                    d = 0;
                            data:
                                    tbfr[d]= finalCode[d];        // LSByte;
                                    d=d+1;
                                    if (d<(tlen))
                                        goto data;
                                }
                                
                                if ((transfer % 2) == 0)
                                {
                                    d = 0;
                            data1:
                                    tbfr[d]= rtbfr[d];        // LSByte;
                                    d=d+1;
                                    if (d<(tlen))
                                        goto data1;
                                }
                                
                                send(StreamSock, tbfr, tlen, 0 );

                                }
                            
                                
                                if(flag==1) // T transmit
                                {
                                    // Parity check matrix     
                                    int H[6][3] = {{1,1,0},
                                                   {1,1,1},
                                                   {1,0,1},
                                                   {1,0,0},
                                                   {0,1,0},
                                                   {0,0,1}};

                                    int r;
                                    int p;
                                    int col;
                                    int row;
                                    int temp;
                                    // retrieve all the data back from client
                                    for (r = 0; r<42; r++)
                                    {
                                        rcbfr[r] = rbfr[r];
                                    }

                                    for (p = 0; p<42;p++)
                                    {
                                        char c = rcbfr[p];
                                        int binary_return[6] = {0,0,0,0,0,0};
                                        int i;
                                        // convert ASCII to binary
                                        for (i = 0; i < 6; ++i) {
                                            binary_return[5-i] = (c >> i) & 1;
                                        }

                                        int syndrome[3] = {0,0,0};
                                        for (col = 0; col < 3; col++)
                                        {
                                            temp = 0;
                                            for (row = 0; row < 6; row++)
                                            {
                                                temp = binary_return[row]*H[row][col] + temp;
                                            }
                                            syndrome[col] = temp;
                                        }
                                        int w;
                                        for (w = 0; w < 3; w++)
                                        {
                                            if ((syndrome[w] % 2)  == 0)
                                            {
                                                syndrome[w] = 0;
                                            }
                                            if ((syndrome[w] % 2)  == 1)
                                            {
                                                syndrome[w] = 1;
                                            }
                                        }

                                        int z = 4*syndrome[0] + 2*syndrome[1] + 1*syndrome[2];
                                        int position;
                                        if (z == 6)
                                        {
                                            position = 0;
                                        }
                                        if (z == 7)
                                        {
                                            position = 1;
                                        }
                                        if (z == 5)
                                        {
                                            position = 2;
                                        }
                                        if (z == 4)
                                        {
                                            position = 3;
                                        }
                                        if (z == 2)
                                        {
                                            position = 4;
                                        }
                                        if (z == 1)
                                        {
                                            position = 5;
                                        }
                                        if (z == 0){
                                            position = 10;
                                        }


                                        if (position != 10)
                                        {
                                            if (binary_return[position] == 1)
                                            {
                                                binary_return[position] = 0;
                                            }
                                            else
                                            {
                                                binary_return[position] = 1;
                                            }
                                        }

                                        int fdec = 32*binary_return[0]+16*binary_return[1]+
                                                   8*binary_return[2]+4*binary_return[3]+
                                                   2*binary_return[4]+1*binary_return[5];

                                        rtbfr[p] = fdec;
                                    }
                                    
                                }
                               
                            }
            
                        else if(rlen < 0)
                            {
                            closesocket( StreamSock );
                            StreamSock = SOCKET_ERROR;
                            }
                        }

            }  // end while(1)
}   // end

// DelayMsec( )   software millisecond delay
void DelayMsec(unsigned int msec)
{
unsigned int tWait, tStart;

    tWait=(SYS_FREQ/2000)*msec;
    tStart=ReadCoreTimer();
    while((ReadCoreTimer()-tStart)<tWait);
}