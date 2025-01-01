#include <reg52.h>

// ����ܶ����0-9��
unsigned char code SEG_CODE[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};

// IO�ڶ���
sbit NORTH_RED = P1^0;
sbit NORTH_YELLOW = P1^1;
sbit NORTH_GREEN = P1^2;
sbit EAST_RED = P1^3;
sbit EAST_YELLOW = P1^4;
sbit EAST_GREEN = P1^5;

// λѡ����
sbit DIGIT_NORTH_1 = P2^0;
sbit DIGIT_NORTH_2 = P2^1;
sbit DIGIT_EAST_1 = P2^2;
sbit DIGIT_EAST_2 = P2^3;

// ��ť����
sbit BTN_NS_GREEN = P2^4; // �ϱ��̵ư�ť
sbit BTN_EW_GREEN = P2^5; // �����̵ư�ť
sbit BTN_ALL_RED = P2^6;  // ȫ��ư�ť

// ȫ�ֱ���
unsigned char green_time = 5;
unsigned char yellow_time = 3;
unsigned char red_time = 8;
unsigned char state = 0;
unsigned char time_left = 0;
bit is_paused = 0;  // ��־�Ƿ���ͣ

// ��ʱ���жϷ�����
void timer0_ISR() interrupt 1 {
    static unsigned int count = 0;
    TH0 = 0xFC;  // ��װ������ֵ
    TL0 = 0x66;
    count++;
    if (count >= 1000 && !is_paused) { // ÿ�룬��δ��ͣ
        count = 0;
        if (time_left > 0) {
            time_left--;
        }
    }
}


// ��ʱ����
void delayms(unsigned int ms) {
    unsigned int i, j;
    for (i = 0; i < ms; i++) {
        for (j = 0; j < 110; j++);
    }
}

// ��ͨ��״̬���º���
void update_lights() {
    if (is_paused) return; // ��ͣʱ�����µƹ�
    switch (state) {
        case 0: // �ϱ���ƣ������̵�
            NORTH_RED = 1; NORTH_YELLOW = 0; NORTH_GREEN = 0;
            EAST_RED = 0; EAST_YELLOW = 0; EAST_GREEN = 1;
            time_left = green_time;
            break;
        case 1: // �ϱ���ƣ������Ƶ�
            NORTH_RED = 1; NORTH_YELLOW = 0; NORTH_GREEN = 0;
            EAST_RED = 0; EAST_YELLOW = 1; EAST_GREEN = 0;
            time_left = yellow_time;
            break;
        case 2: // �ϱ��̵ƣ��������
            NORTH_RED = 0; NORTH_YELLOW = 0; NORTH_GREEN = 1;
            EAST_RED = 1; EAST_YELLOW = 0; EAST_GREEN = 0;
            time_left = green_time;
            break;
        case 3: // �ϱ��Ƶƣ��������
            NORTH_RED = 0; NORTH_YELLOW = 1; NORTH_GREEN = 0;
            EAST_RED = 1; EAST_YELLOW = 0; EAST_GREEN = 0;
            time_left = yellow_time;
            break;
    }
}

// �������ʾ
void display(unsigned char num, bit is_north, bit is_tens) {
    unsigned char digit = SEG_CODE[num]; // ��ȡ����
    P0 = digit;                         // ������뵽�����
    if (is_north) {
        if (is_tens) DIGIT_NORTH_2 = 0; // �ϱ�ʮλ
        else DIGIT_NORTH_1 = 0;         // �ϱ���λ
    } else {
        if (is_tens) DIGIT_EAST_2 = 0;  // ����ʮλ
        else DIGIT_EAST_1 = 0;          // ������λ
    }
    delayms(5);                           // ��ʾ�ȶ�
    if (is_north) {
        if (is_tens) DIGIT_NORTH_2 = 1; // �ָ�Ϊ1
        else DIGIT_NORTH_1 = 1;
    } else {
        if (is_tens) DIGIT_EAST_2 = 1;
        else DIGIT_EAST_1 = 1;
    }
}

// ˢ�������
void refresh_display(unsigned char north_time, unsigned char east_time) {
    // ��ʾ�ϱ������
    display(north_time / 10, 1, 0); // ʮλ
    display(north_time % 10, 1, 1); // ��λ

    // ��ʾ���������
    display(east_time / 10, 0, 0); // ʮλ
    display(east_time % 10, 0, 1); // ��λ
}

// �ⲿ�ж�0����ͣ����
void ext0_ISR() interrupt 0 {
    is_paused = 1;  // ����Ϊ��ͣ
    time_left = 0;  // ��ʱ������
    refresh_display(0, 0); // �ް�ť����ʱ��ʾ0
    while (INT1 != 0) {
        // ��鰴ť״̬���ı�Ƶ�״̬
        if (BTN_NS_GREEN == 0) { // �ϱ��̵�
            NORTH_RED = 0; NORTH_YELLOW = 0; NORTH_GREEN = 1;
            EAST_RED = 1; EAST_YELLOW = 0; EAST_GREEN = 0;
        } else if (BTN_EW_GREEN == 0) { // �����̵�
            NORTH_RED = 1; NORTH_YELLOW = 0; NORTH_GREEN = 0;
            EAST_RED = 0; EAST_YELLOW = 0; EAST_GREEN = 1;
        } else if (BTN_ALL_RED == 0) { // ȫ���
            NORTH_RED = 1; NORTH_YELLOW = 0; NORTH_GREEN = 0;
            EAST_RED = 1; EAST_YELLOW = 0; EAST_GREEN = 0;
        }
    }
}

// �ⲿ�ж�1����������
void ext1_ISR() interrupt 2 {
    is_paused = 0;  // �ָ�����
    time_left = 0;  // ��ʱ������
}

// ������
void main() {
    unsigned char north_display_time, east_display_time;
    TMOD = 0x01;   // ��ʱ��0��ģʽ1
    TH0 = 0xFC;    // ��ֵ
    TL0 = 0x66;
    ET0 = 1;       // ������ʱ���ж�
    EX0 = 1;       // �����ⲿ�ж�0
    EX1 = 1;       // �����ⲿ�ж�1
    IT0 = 1;       // �½��ش����ж�0
    IT1 = 1;       // �½��ش����ж�1
    EA = 1;        // ���жϿ���
    TR0 = 1;       // ������ʱ��0

    while (1) {
        update_lights();
        while (time_left > 0 && !is_paused) {
            if (state == 0) { // �ϱ���ƣ������̵�
                north_display_time = red_time - green_time + time_left;
                east_display_time = time_left;
            } else if (state == 1 || state == 3) { // �ϱ���ƣ������Ƶ�
                north_display_time = time_left;
                east_display_time = time_left;
            } else if (state == 2) { // �ϱ��̵ƣ��������
                north_display_time = time_left;
                east_display_time = red_time - green_time + time_left;
            }
            refresh_display(north_display_time, east_display_time); // ˢ����ʾ
        }
        if (!is_paused) {
            state = (state + 1) % 4; // �л�����һ��״̬
        }
    }
}
