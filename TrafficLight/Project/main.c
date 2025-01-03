#include <reg52.h>

// 数码管段码表（0-9）
unsigned char code SEG_CODE[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};

// IO口定义
sbit NORTH_RED = P1^0;
sbit NORTH_YELLOW = P1^1;
sbit NORTH_GREEN = P1^2;
sbit EAST_RED = P1^3;
sbit EAST_YELLOW = P1^4;
sbit EAST_GREEN = P1^5;

// 位选定义
sbit DIGIT_NORTH_1 = P2^0;
sbit DIGIT_NORTH_2 = P2^1;
sbit DIGIT_EAST_1 = P2^2;
sbit DIGIT_EAST_2 = P2^3;

// 按钮定义
sbit BTN_NS_GREEN = P2^4; // 南北绿灯按钮
sbit BTN_EW_GREEN = P2^5; // 东西绿灯按钮
sbit BTN_ALL_RED = P2^6;  // 全红灯按钮

// 全局变量
unsigned char green_time = 5;
unsigned char yellow_time = 3;
unsigned char red_time = 8;
unsigned char state = 0;
unsigned char time_left = 0;
bit is_paused = 0;  // 标志是否暂停

// 定时器中断服务函数
void timer0_ISR() interrupt 1 {
    static unsigned int count = 0;
    TH0 = 0xFC;  // 重装计数初值
    TL0 = 0x66;
    count++;
    if (count >= 1000 && !is_paused) { // 每秒，且未暂停
        count = 0;
        if (time_left > 0) {
            time_left--;
        }
    }
}


// 延时函数
void delayms(unsigned int ms) {
    unsigned int i, j;
    for (i = 0; i < ms; i++) {
        for (j = 0; j < 110; j++);
    }
}

// 交通灯状态更新函数
void update_lights() {
    if (is_paused) return; // 暂停时不更新灯光
    switch (state) {
        case 0: // 南北红灯，东西绿灯
            NORTH_RED = 1; NORTH_YELLOW = 0; NORTH_GREEN = 0;
            EAST_RED = 0; EAST_YELLOW = 0; EAST_GREEN = 1;
            time_left = green_time;
            break;
        case 1: // 南北红灯，东西黄灯
            NORTH_RED = 1; NORTH_YELLOW = 0; NORTH_GREEN = 0;
            EAST_RED = 0; EAST_YELLOW = 1; EAST_GREEN = 0;
            time_left = yellow_time;
            break;
        case 2: // 南北绿灯，东西红灯
            NORTH_RED = 0; NORTH_YELLOW = 0; NORTH_GREEN = 1;
            EAST_RED = 1; EAST_YELLOW = 0; EAST_GREEN = 0;
            time_left = green_time;
            break;
        case 3: // 南北黄灯，东西红灯
            NORTH_RED = 0; NORTH_YELLOW = 1; NORTH_GREEN = 0;
            EAST_RED = 1; EAST_YELLOW = 0; EAST_GREEN = 0;
            time_left = yellow_time;
            break;
    }
}

// 数码管显示
void display(unsigned char num, bit is_north, bit is_tens) {
    unsigned char digit = SEG_CODE[num]; // 获取段码
    P0 = digit;                         // 输出段码到数码管
    if (is_north) {
        if (is_tens) DIGIT_NORTH_2 = 0; // 南北十位
        else DIGIT_NORTH_1 = 0;         // 南北个位
    } else {
        if (is_tens) DIGIT_EAST_2 = 0;  // 东西十位
        else DIGIT_EAST_1 = 0;          // 东西个位
    }
    delayms(5);                           // 显示稳定
    if (is_north) {
        if (is_tens) DIGIT_NORTH_2 = 1; // 恢复为1
        else DIGIT_NORTH_1 = 1;
    } else {
        if (is_tens) DIGIT_EAST_2 = 1;
        else DIGIT_EAST_1 = 1;
    }
}

// 刷新数码管
void refresh_display(unsigned char north_time, unsigned char east_time) {
    // 显示南北数码管
    display(north_time / 10, 1, 0); // 十位
    display(north_time % 10, 1, 1); // 个位

    // 显示东西数码管
    display(east_time / 10, 0, 0); // 十位
    display(east_time % 10, 0, 1); // 个位
}

// 外部中断0，暂停功能
void ext0_ISR() interrupt 0 {
    is_paused = 1;  // 设置为暂停
    time_left = 0;  // 计时器归零
    refresh_display(0, 0); // 无按钮按下时显示0
    while (INT1 != 0) {
        // 检查按钮状态并改变灯的状态
        if (BTN_NS_GREEN == 0) { // 南北绿灯
            NORTH_RED = 0; NORTH_YELLOW = 0; NORTH_GREEN = 1;
            EAST_RED = 1; EAST_YELLOW = 0; EAST_GREEN = 0;
        } else if (BTN_EW_GREEN == 0) { // 东西绿灯
            NORTH_RED = 1; NORTH_YELLOW = 0; NORTH_GREEN = 0;
            EAST_RED = 0; EAST_YELLOW = 0; EAST_GREEN = 1;
        } else if (BTN_ALL_RED == 0) { // 全红灯
            NORTH_RED = 1; NORTH_YELLOW = 0; NORTH_GREEN = 0;
            EAST_RED = 1; EAST_YELLOW = 0; EAST_GREEN = 0;
        }
    }
}

// 外部中断1，启动功能
void ext1_ISR() interrupt 2 {
    is_paused = 0;  // 恢复运行
    time_left = 0;  // 计时器归零
}

// 主程序
void main() {
    unsigned char north_display_time, east_display_time;
    TMOD = 0x01;   // 定时器0，模式1
    TH0 = 0xFC;    // 初值
    TL0 = 0x66;
    ET0 = 1;       // 开启定时器中断
    EX0 = 1;       // 开启外部中断0
    EX1 = 1;       // 开启外部中断1
    IT0 = 1;       // 下降沿触发中断0
    IT1 = 1;       // 下降沿触发中断1
    EA = 1;        // 总中断开关
    TR0 = 1;       // 启动定时器0

    while (1) {
        update_lights();
        while (time_left > 0 && !is_paused) {
            if (state == 0) { // 南北红灯，东西绿灯
                north_display_time = red_time - green_time + time_left;
                east_display_time = time_left;
            } else if (state == 1 || state == 3) { // 南北红灯，东西黄灯
                north_display_time = time_left;
                east_display_time = time_left;
            } else if (state == 2) { // 南北绿灯，东西红灯
                north_display_time = time_left;
                east_display_time = red_time - green_time + time_left;
            }
            refresh_display(north_display_time, east_display_time); // 刷新显示
        }
        if (!is_paused) {
            state = (state + 1) % 4; // 切换到下一个状态
        }
    }
}
