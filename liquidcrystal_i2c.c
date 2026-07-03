#include "liquidcrystal_i2c.h"

// Usiamo 'extern' per dire al compilatore che la variabile hi2c1
// è già stata creata e configurata nel file main.c
extern I2C_HandleTypeDef hi2c1;

void lcd_send_cmd (char cmd) {
    char data_u, data_l;
    uint8_t data_t[4];
    data_u = (cmd & 0xf0);
    data_l = ((cmd << 4) & 0xf0);

    data_t[0] = data_u | 0x0C;  // en=1, rs=0, backlight=1
    data_t[1] = data_u | 0x08;  // en=0, rs=0, backlight=1
    data_t[2] = data_l | 0x0C;  // en=1, rs=0, backlight=1
    data_t[3] = data_l | 0x08;  // en=0, rs=0, backlight=1
    HAL_I2C_Master_Transmit(&hi2c1, LCD_ADDR, (uint8_t *)data_t, 4, 100);
}

void lcd_send_data (char data) {
    char data_u, data_l;
    uint8_t data_t[4];
    data_u = (data & 0xf0);
    data_l = ((data << 4) & 0xf0);

    data_t[0] = data_u | 0x0D;  // en=1, rs=1, backlight=1
    data_t[1] = data_u | 0x09;  // en=0, rs=1, backlight=1
    data_t[2] = data_l | 0x0D;  // en=1, rs=1, backlight=1
    data_t[3] = data_l | 0x09;  // en=0, rs=1, backlight=1
    HAL_I2C_Master_Transmit(&hi2c1, LCD_ADDR, (uint8_t *)data_t, 4, 100);
}

void lcd_init (void) {
    HAL_Delay(50);
    lcd_send_cmd(0x30); // Sincronizzazione iniziale in modalità 8-bit
    HAL_Delay(5);
    lcd_send_cmd(0x30);
    HAL_Delay(1);
    lcd_send_cmd(0x32); // Forza il passaggio stabile alla modalità 4-bit
    HAL_Delay(10);

    lcd_send_cmd(0x28); // Configurazione: 4-bit, 2 linee di testo, font 5x8
    HAL_Delay(1);
    lcd_send_cmd(0x0C); // Display ON, Cursore OFF
    HAL_Delay(1);
    lcd_send_cmd(0x01); // Cancella completamente lo schermo da vecchi residui
    HAL_Delay(3);
    lcd_send_cmd(0x06); // Incremento automatico del cursore a destra
    HAL_Delay(1);
}

void lcd_send_string (char *str) {
    while (*str) lcd_send_data(*str++);
}

void lcd_put_cur(int row, int col) {
    switch (row) {
        case 0: col |= 0x80; break; // Riga 1
        case 1: col |= 0xC0; break; // Riga 2
    }
    lcd_send_cmd(col);
}

void lcd_clear(void) {
    lcd_send_cmd(0x01); // Invia il comando hardware di Clear Display
    HAL_Delay(2);       // Sosta obbligatoria: il chip LCD impiega circa 1.6ms per ripulire la DDRAM
}
