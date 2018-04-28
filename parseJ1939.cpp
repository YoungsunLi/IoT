//
// Created by lianzeng on 18-4-19.
//


#include <cstdio>
#include <string>
#include <cassert>
#include "canBuffer.hpp"
#include "formula1939.hpp"
#include "reportJ1939.hpp"
#include "logger.hpp"

using std::string;

extern J1939Reports g_j1939Report;
extern Logger g_logger;

inline int a2hex(char c)
{
    if( '0'<= c && c <= '9') return c - '0';
    else if('A'<= c && c <= 'F') return c - 'A' + 10;
    else
    {
        printf("\n error:invalid ascii,not a number. \n");
        return 0;
    }
}



int byte2hex(char H, char L)
{
    return ((a2hex(H) << 4) + a2hex(L)) & 0xFF;
}

int byteIndex(const string& d, int i)
{
    assert(1<= i && i <= 8);
    return byte2hex(d[8+i*3],d[9+i*3]);
}



float limit(float v, float min, float max)
{
    if(v < min) return min;
    else if(v > max) return max;
    else return v;
}

void parkingBrakeSwitch(char byte)
{
    string status;
    int bit34 = (byte>>2)&0x3;//SPN70
    switch(bit34)
    {
        case 0: {status = "parkBrakeNotSet";break;}
        case 1: {status = "parkBrakeSet";break;}
        case 2: {status = "Error";break;}
        case 3: {status = "NotAvailbe";break;}
        default:{status = "NotAvailbe";break;}
    }
    printf("parkingBrake:%s \n",status.data());
}

void calcVehicleSpeed(const string &datas)
{
    //Vehicle Speed,km/h, SPN = 84,PGN = 0xFEF1=65265;CCVS,Byte2-3
    int b2 = byteIndex(datas,2);
    int b3 = byteIndex(datas,3);
    float vehicleSpeed = ((b3<<8)+ b2)/256.0f;
    vehicleSpeed = limit(vehicleSpeed, 0, 250.996);
    g_j1939Report.vehicleSpeed = vehicleSpeed;
    printf(" vehicleSpeed=%.3f km/h \n", vehicleSpeed);
}

void brakeSwitch(char byte)
{
    string state;
    char b56 = (byte>>4) & 0x3;
    switch(b56)
    {
        case 0:{state = "brakePedalReleased";break;}
        case 1:{state = "brakePedalDepressed";break;}
        case 2:{state = "Error";break;}
        case 3:{state = "NotAvaible";break;}
        default:break;
    }
    printf("brakeSwitch:%s \n",state.data());
}

void cruiseControlVehicleSpeed(const string &datas)
{

    const int CAN_ID_LEN = 8;//29bit canid has 8 ascii char.

    int DLC = a2hex(datas[9]);//TODO:data length,adapt for ELM327

    if (datas.find(' ') != CAN_ID_LEN || DLC != 8 )
    {
        printf("\n discard \n");//not 29bit canid or payload !=8 bytes.
        return;
    }

#if 0
    int PF = (a2hex(datas[2]) << 4) + a2hex(datas[3]);
    int PS = (a2hex(datas[4]) << 4) + a2hex(datas[5]);
    int PGN = (PF < 240) ? PF : ((PF << 8) + PS);
    //printf("\n DLC=%d,PF=%X,PS=%X,PGN=%X \n", DLC, PF, PS, PGN);
    if(PGN != 0xFEF1)
    {
        printf("\n error PGN:0x%X != 0xFEF1 \n",PGN);
        return;
    }
#endif
    int b1 = byteIndex(datas,1);
    parkingBrakeSwitch(b1);
    int b4 = byteIndex(datas,4);
    brakeSwitch(b4);
    calcVehicleSpeed(datas);
}



void calcEngineSpeed(const string &datas)
{
    if (a2hex(datas[9]) != 8 )
    {
        printf("\n discard \n");
        return;
    }
    int b4 = byteIndex(datas,4);
    int b5 = byteIndex(datas,5);
    float espeed = ((b5<<8)+b4)*0.125f;
    espeed = limit(espeed,0,8031.875);
    g_j1939Report.engSpeed = espeed;
    printf(" engineSpeed=%.3f rpm \n", espeed);
}

void calcEngineLoad(const string &datas)
{
    if (a2hex(datas[9]) != 8 )
    {
        printf("\n discard \n");
        return;
    }
    int b3 = byteIndex(datas, 3);
    int eload = limit(b3,0,125);
    g_j1939Report.engLoad = eload;
    printf(" engine load= %d%%\n", eload);
}

void calcDistance(const string& datas)
{
    int b1 = byteIndex(datas,1);
    int b2 = byteIndex(datas,2);
    int b3 = byteIndex(datas,3);
    long b4 = byteIndex(datas,4);//need 64bit.
    float totalDistance = ((b4<<24)+(b3<<16)+(b2<<8)+b1)*5/1000.0;
    printf("total vehicle distance=%.3f km\n",totalDistance);

    int b5 = byteIndex(datas,5);
    int b6 = byteIndex(datas,6);
    int b7 = byteIndex(datas,7);
    long b8 = byteIndex(datas,8);
    float tripDistance = ((b8<<24)+(b7<<16)+(b6<<8)+b5)*5/1000.0;
    printf("trip distance=%.3f km\n",tripDistance);
}

void calcEngineTemperature(const string& datas)
{
    int b1 = byteIndex(datas,1);
    int coolT = b1 -40;//offset=-40deg
    coolT = limit(coolT,-40,210);
    printf("engine coolant temperature=%d degC\n",coolT);
    int b2 = byteIndex(datas,2);
    int fuelT = b2 - 40;
    fuelT = limit(fuelT,-40,210);
    printf("engine Fuel temperature=%d degC\n",fuelT);
}

void calcAirInletTemperature(const string& datas)
{
    int b6 = byteIndex(datas,6);
    int ait = b6 - 40;
    ait = limit(ait,-40,210);
    printf("air inlet temperature=%d degC\n",ait);
}

void calcBatteryVoltage(const string& datas)
{
    int b5 = byteIndex(datas,5);
    int b6 = byteIndex(datas,6);
    float evol = ((b6<<8)+b5)*0.05f;
    evol = limit(evol,0,3212.75);
    printf("electrical Voltage=%.3fV\n",evol);
    int b7 = byteIndex(datas,7);
    int b8 = byteIndex(datas,8);
    float bvol = ((b8<<8)+b7)*0.05f;
    bvol = limit(bvol,0,3212.75);
    printf("battery voltage=%.3fV\n",bvol);
}

void calcEngineInletAir(const string& datas)
{
    int b3 = byteIndex(datas,3);
    int b4 = byteIndex(datas,4);
    float mfr = ((b4<<8)+b3)*0.05f;
    mfr = limit(mfr, 0, 3212.75);
    printf("engine inlet air Mass Flow rate=%.3fkg/h\n",mfr);
}

void countFaultNum(const string& datas)
{
    int b6 = byteIndex(datas,6);
    int faultNum = b6&0x7F;
    printf("faultNum=%d \n",faultNum);
}

void calcVehicleHours(const string& datas)
{
    int b1 = byteIndex(datas,1);
    int b2 = byteIndex(datas,2);
    int b3 = byteIndex(datas,3);
    long b4 = byteIndex(datas,4);//need 64bit.
    float totalHours = ((b4<<24)+(b3<<16)+(b2<<8)+b1)*0.05;
    totalHours = limit(totalHours, 0, 210554060.75);
    printf("totalVehicleHours=%.3f hr\n",totalHours);
}

void calcFuelConsumption(const string& datas)
{
    int b1 = byteIndex(datas,1);
    int b2 = byteIndex(datas,2);
    int b3 = byteIndex(datas,3);
    long b4 = byteIndex(datas,4);//need 64bit.
    float tripFuel = ((b4<<24)+(b3<<16)+(b2<<8)+b1)*0.5;
    printf("EngTripFuelUsed=%.3f L\n",tripFuel);

    int b5 = byteIndex(datas,5);
    int b6 = byteIndex(datas,6);
    int b7 = byteIndex(datas,7);
    long b8 = byteIndex(datas,8);
    float totalFuel = ((b8<<24)+(b7<<16)+(b6<<8)+b5)*0.5;
    printf("EngTotalFuelUsed=%.3f L\n",totalFuel);
}

void calcEngFuelRate(const string& datas)
{
    int b1 = byteIndex(datas,1);
    int b2 = byteIndex(datas,2);
    float fuelR = ((b2<<8)+b1)*0.05;
    fuelR = limit(fuelR,0,3212.75);
    printf("EngFuelRate=%.3f L/h\n",fuelR);
}

void calcJ1939ParasOverCanBuffer(const CanBusBuffer& canBusBuffer)
{//this func is called periodically or when buffer is full;

    //extract the specific can frame from buffer to parse, result is in type order,not time order
    //18 indicate priority=6
    static const Formula1939 formula1939[] =
     {
        {"18FEF100",cruiseControlVehicleSpeed},//SPN84,PGN65265,SAE-j1939-71,"CCVS" 车速，刹车
        {"0CF00400",calcEngineSpeed}, //SPN190,PGN61444,"EEC1" 发动机转速RPM，
        {"0CF00300",calcEngineLoad}, //SPN92,PGN61443,"EEC2" 发动机负荷，
        {"18FEC100",calcDistance}, //SPN917,SPN918,PGN=65217,"VDHR" 里程，
        {"18FEEE00",calcEngineTemperature},//SPN110,SPN174,PGN=65262,"ET1"冷却液温度，发动机燃油温度
        {"18FEF500",calcAirInletTemperature},//SPN172,PGN65269,"AMB"进气温度，
        {"18FEF700",calcBatteryVoltage},//SPN158,PGN65271,"VEP1"电瓶电压，
        {"0CF00A00",calcEngineInletAir},//SPN132,PGN61450,"EGF1"发动机进气流量，
        {"18FECA00",countFaultNum},//diagnose message,"DM1"故障码
        {"18FEE700",calcVehicleHours},//SPN246,PGN65255 开车时间
        {"18FEE900",calcFuelConsumption},//SPN182,SPN250,PGN65257油耗
        {"18FEF200",calcEngFuelRate}, //SPN183,PGN65266 燃油率

     };

    static int frameIdx = 0;

    const int max_sample_per_second = 10;

    if(canBusBuffer.empty()) return;//no need to calculate if no canbus received
    g_logger.setUpdateFlag();

    for(int i = 0; i < sizeof(formula1939)/sizeof(formula1939[0]); i++)
    {
        int startPos = 0;
        for(int count = 0; count < max_sample_per_second;  count++)
        {
            string oneFrame = canBusBuffer.extractCanFrame(formula1939[i].canId, startPos);
            if(oneFrame.size() == CAN_FRAME_LEN )
            {
                printf(" %d: %s \n",frameIdx++,oneFrame.data());
                formula1939[i].formula(oneFrame);
            }
            else
                break;
        }
    }
}
