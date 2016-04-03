// PIC32 Server - Microchip BSD stack socket API
// MPLAB X C32 Compiler     PIC32MX795F512L
// Microchip DM320004 Ethernet Starter Board
/*
    Author: Harish Boggarapu
    Title: Linear Block Code Error Correction

    This project implements a Hamming linear block code error detection and
    correction scheme. The message "EE is my avocation" is transmitted
    between a PC and PIC32 Ethernet sever. Client used for this project
    is Client3 v16 provided by Professor Dr.Silage. The Client adds up to 10 bits
    of errors and retransmits back to the server. A (6,3) Hamming linear block code
    is used with the generator matrix G, parity check matrix H and decode matrix Dc matrix
    shown below .

    G = {{1,0,0,1,1,0},
         {0,1,0,1,1,1},
         {0,0,1,1,0,1}}

    H = {{1,1,0},
        {1,1,1},
        {1,0,1},
        {1,0,0},
        {0,1,0},
        {0,0,1}}

    Dc = {{1,0,0},
          {0,1,0},
          {0,0,1},
          {0,0,0},
          {0,0,0},
          {0,0,0}}

*/


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
            char rtbfr[18];
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
                                                codeWord[col] = temp % 2;
                                            }
                                            gcounter = gcounter + 3;

                                            int dec = 32*codeWord[0] + 16*codeWord[1] + 8*codeWord[2] + 4*codeWord[3] + 2*codeWord[4] + 1*codeWord[5];
                                            finalCode[fcounter] = dec;
                                            fcounter++;
                                        }
                                    }

                                    d = 0;
                            data:
                                    tbfr[d]= finalCode[d];
                                    d=d+1;
                                    if (d<(tlen))
                                        goto data;
                                }
                                // load and transmit corrected data
                                if ((transfer % 2) == 0)
                                {
                                    d = 0;
                            data1:
                                    tbfr[d]= rtbfr[d];
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
                                    // Decode matrix
                                    int Dc[6][3] = {{1,0,0},
                                                   {0,1,0},
                                                   {0,0,1},
                                                   {0,0,0},
                                                   {0,0,0},
                                                   {0,0,0}};

                                    int r;
                                    int p = 0;
                                    int col;
                                    int row;
                                    int temp;
                                    int count;
                                    // retrieve all the data back from client
                                    for (r = 0; r<42; r++)
                                    {
                                        rcbfr[r] = rbfr[r];
                                    }


                                    for (count=0; count<6;count++)
                                    {
                                        int decode_binary[21] = {0,0,0,0,0,0,0,
                                                                 0,0,0,0,0,0,0,
                                                                 0,0,0,0,0,0,0};
                                        int dcount = 0;
                                        int count1;
                                        int aCount = 0;
                                        for (count1 = 0; count1 < 7; count1++)
                                        {
                                            char c = rcbfr[p];
                                            p++;
                                            int binary_return[6] = {0,0,0,0,0,0};
                                            int i;
                                            // convert ASCII to binary
                                            for (i = 0; i < 6; ++i) {
                                                binary_return[5-i] = (c >> i) & 1;
                                            }

                                            int syndrome[3] = {0,0,0};
                                            // get syndrome bits for identifying error bit position
                                            for (col = 0; col < 3; col++)
                                            {
                                                temp = 0;
                                                for (row = 0; row < 6; row++)
                                                {
                                                    temp = binary_return[row]*H[row][col] + temp;
                                                }
                                                syndrome[col] = temp % 2;
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

                                            // fix corrupted bit
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

                                            int dataBits[3] = {0,0,0};
                                            // get data bits
                                            for (col = 0; col < 3; col++)
                                            {
                                                temp = 0;
                                                for (row = 0; row < 6; row++)
                                                {
                                                    temp = binary_return[row]*Dc[row][col] + temp;
                                                }
                                                dataBits[col] = temp % 2;
                                            }

                                            int dbits;

                                            for (dbits = 0; dbits < 3; dbits++)
                                            {
                                                decode_binary[dcount] = dataBits[dbits];
                                                dcount++;
                                            }

                                        }

                                        int asciiData;
                                        int asciiData1;
                                        int asciiCount = 0;
                                        for (asciiData=0;asciiData<3;asciiData++)
                                        {
                                           int asciiValue[7] = {0,0,0,0,0,0,0};
                                           for (asciiData1=0;asciiData1<7;asciiData1++)
                                            {
                                                asciiValue[asciiData1] = decode_binary[asciiCount];
                                                asciiCount++;
                                            }

                                            int fdec = 0;
                                            fdec = 64*asciiValue[0]+32*asciiValue[1]+16*asciiValue[2]+
                                                    8*asciiValue[3]+4*asciiValue[4]+2*asciiValue[5]+ 1*asciiValue[6];
                                            printf("%d,",fdec);
                                            rtbfr[aCount] = fdec;
                                            aCount++;
                                        }
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