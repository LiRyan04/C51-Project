#include <reg52.h>

// 类型定义
typedef unsigned char uchar;
typedef unsigned int uint;

sbit duxuan = P2^1;
sbit weixuan = P2^2;

// 延时函数
void delayms(uint ms) {
    uint i, j;
    for (i = ms; i > 0; i--)
        for (j = 110; j > 0; j--);
}

// 数码管显示函数
void display(uchar num[]) {
    uchar segcode[] = {0x3f, 0x06, 0x5b, 0x4f, 0x66, 0x6d, 0x7d, 0x07, 0x7f, 0x6f};
    uchar select_data[] = {0xfe, 0xfd, 0xfb, 0xf7, 0xef, 0xdf, 0xbf, 0x7f};
    uint i;
    for (i = 0; i < 8; i++) {
        duxuan = 1; P0 = 0x00; duxuan = 0; // 清空段选
        weixuan = 1; P0 = 0xff; weixuan = 0; // 清空位选
        weixuan = 1; P0 = select_data[i]; weixuan = 0; // 位选
        duxuan = 1; P0 = segcode[num[i]]; duxuan = 0; // 段选
        delayms(1);
    }
}

// 清零显示函数
void clearDisplay(uchar* display_data) {
    uchar i;
    for (i = 0; i < 8; i++) {
        display_data[i] = 0; // 清零数组
    }
}

// 矩阵键盘扫描函数
uchar matr_keyscan() {
    uchar temp, keycode = 0xff;
    // 按行扫描
    uchar scan_row[4] = {0xfe, 0xfd, 0xfb, 0xf7};
    uchar key_map[4][4] = {
        {7, 8, 9, '/'}, 
        {4, 5, 6, '*'}, 
        {1, 2, 3, '-'}, 
        {'C', 0, '=', '+'}
    };

    unsigned char row, col;
    for (row = 0; row < 4; row++) {
        P1 = scan_row[row];
        temp = P1 & 0xf0;
        if (temp != 0xf0) {
            delayms(10); // 消抖
            temp = P1 & 0xf0;
            if (temp != 0xf0) {
                for (col = 0; col < 4; col++) {
                    if ((temp & (0x10 << col)) == 0) {
                        while ((P1 & 0xf0) != 0xf0); // 等待释放
                        return key_map[row][col];
                    }
                }
            }
        }
    }
    return 0xff;
}

// 计算函数
int calculate(int a, int b, char op) {
    switch (op) {
        case '+': return a + b;
        case '-': return a - b;
        case '*': return a * b;
        case '/': return b != 0 ? a / b : 0; // 防止除零错误
        default: return 0;
    }
}

void main() {
    uchar display_data[8] = {0, 0, 0, 0, 0, 0, 0, 0}; 
    int operand1 = 0, operand2 = 0;
    char operator = 0;
    int result = 0;
    char input_stage = 0; // 0: 输入第一个操作数, 1: 输入操作符, 2: 输入第二个操作数
    int temp;
    uint i;

    while (1) {
        uchar key = matr_keyscan();
        if (key != 0xff) {
            if (key == 'C') {
                clearDisplay(display_data);
                operand1 = operand2 = result = 0;
                operator = 0;
                input_stage = 0;
            } else if (key == '=') {
                if (input_stage == 2) {
                    result = calculate(operand1, operand2, operator);
                    operand1 = result;
                    operand2 = 0;
                    input_stage = 1;
                }
            } else if (key >= 0 && key <= 9) {
                if (input_stage == 0) {
                    operand1 = operand1 * 10 + key;
                } else if (input_stage == 2) {
                    operand2 = operand2 * 10 + key;
                }
            } else {
                if (input_stage == 0) {
                    input_stage = 1;
                }
                if (input_stage == 1) {
                    operator = key;
                    input_stage = 2;
                    clearDisplay(display_data); // 输入运算符后清零显示
                }
            }

            // 更新显示数据
            temp = input_stage == 2 ? operand2 : operand1;
            for (i = 0; i < 8; i++) {
                display_data[7 - i] = temp % 10;
                temp /= 10;
            }
        }
        display(display_data); // 实时显示
    }
}
