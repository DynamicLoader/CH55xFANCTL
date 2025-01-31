#include "CH554.H"
#include "DEBUG.H"
#include "DS18B20.H"

// DS18B20ָ���
#define DS18B20_SKIP_ROM 0xCC
#define DS18B20_READ_ROM 0x33
#define DS18B20_CONVERT_T 0x44
#define DS18B20_READ_SCRATCHPAD 0xBE
#define DS18B20_MATCH_ROM 0x55

/**
 * @brief  �����߳�ʼ��
 * @param  ��
 * @retval �ӻ���Ӧλ��0Ϊ��Ӧ��1Ϊδ��Ӧ
 */
bit OneWire_Init(void)
{
    unsigned char i;
    E_DIS = 1;
    OneWire_DQ = 0;
    // i = 247;
    // while (--i);    // Delay 500us
    mDelayuS(750);
    OneWire_DQ = 1;
    // i = 32;
    // while (--i);    // Delay 70us
    mDelayuS(60);
    while(OneWire_DQ && i < 200){
        mDelayuS(1);
        i++;
    }
    // i = 247;
    // while (--i);    // Delay 500us
    mDelayuS(500);
    E_DIS = 0;
    return i >= 200; 
}
/**
 * @brief  �����߷���һλ
 * @param  Bit Ҫ���͵�λ
 * @retval ��
 */
void OneWire_SendBit(unsigned char Bit)
{
    if(Bit){
        OneWire_DQ = 0;
        mDelayuS(4);
        OneWire_DQ = 1;
        mDelayuS(60);
    }else{
        OneWire_DQ = 0;
        mDelayuS(60);
        OneWire_DQ = 1;
        mDelayuS(4);
    }
}
/**
 * @brief  �����߽���һλ
 * @param  ��
 * @retval ��ȡ��λ
 */
bit OneWire_ReceiveBit(void)
{
    // unsigned char i;
    bit Bit;
    OneWire_DQ = 0;
    // i = 2;
    // while (--i);    // Delay 5us
    mDelayuS(5);
    OneWire_DQ = 1;
    // i = 2;
    // while (--i);    // Delay 5us
    mDelayuS(4);
    Bit = OneWire_DQ;
    // i = 24;
    // while (--i);    // Delay 50us
    mDelayuS(120);
    return Bit;
}
/**
 * @brief  �����߷���һ���ֽ�
 * @param  Byte Ҫ���͵��ֽ�
 * @retval ��
 */
void OneWire_SendByte(unsigned char Byte)
{
    unsigned char i;
    E_DIS = 1;
    for (i = 0; i < 8; i++) {
        OneWire_SendBit(Byte & (0x01 << i));
    }
    E_DIS = 0;
}
/**
 * @brief  �����߽���һ���ֽ�
 * @param  ��
 * @retval ���յ�һ���ֽ�
 */
unsigned char OneWire_ReceiveByte(void)
{
    unsigned char i;
    unsigned char Byte = 0x00;
    E_DIS = 1;
    for (i = 1; i <= 8; i++) {
        Byte = (UINT8)OneWire_ReceiveBit() << 7 | (Byte >> 1);
    }
    E_DIS = 0;
    return Byte;
}

/**
 * @brief  DS18B20��ʼ�¶�ת��
 * @param  ��
 * @retval ��
 */
void DS18B20_ConvertT(void)
{
    OneWire_Init(); // ��ʼ��OneWire����
    OneWire_SendByte(DS18B20_SKIP_ROM); // ����ROMƥ��
    OneWire_SendByte(DS18B20_CONVERT_T); // �����¶�ת������
}

/**
 * @brief  DS18B20��ȡ�¶ȣ�����ROM�룩
 * @param  DS18B20��ROM��
 * @retval �¶���ֵ
 */
UINT16 DS18B20_ReadT(unsigned char* ROM)
{
    unsigned char i;
    // unsigned char TLSB, TMSB;
    UINT8 Temp[2];
    if(OneWire_Init()){
        return 0x7ff;
    }
    if (ROM != NULL) {
        OneWire_SendByte(DS18B20_MATCH_ROM); // ����ƥ��ROM����
        for (i = 0; i < 8; i++) {
            OneWire_SendByte(ROM[i]);
        }
    } else {
        OneWire_SendByte(DS18B20_SKIP_ROM); // ����ROMƥ��
    }

    OneWire_SendByte(DS18B20_READ_SCRATCHPAD); // ��ȡ�¶�����
    // mDelaymS(1);
    Temp[1] = OneWire_ReceiveByte();
    Temp[0] = OneWire_ReceiveByte();
    return *(UINT16*)(&Temp);
}
/**
 * @brief  ��ȡROM��
 * @param  �洢DS18B20 ROM�������
 * @retval None
 */
void DS18B20_ReadRomCode(unsigned char* romcode)
{
    bit ack;
    unsigned char i;
    ack = OneWire_Init();
    if (ack == 0) {
        OneWire_SendByte(DS18B20_READ_ROM); // ���Ͷ�ȡROM����
        // Delay(1);
        mDelaymS(1);
        for (i = 0; i < 8; i++) {
            romcode[i] = OneWire_ReceiveByte(); // ��ȡ64λROM��
        }
    }
}