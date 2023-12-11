#ifndef LCD_H
#define LCD_H
// 128x64-06d https://aliexpress.icom/item/32800995659.html

// these are PC pins
#define LCD_DATA_CMD_PIN   4
#define LCD_NRESET_PIN     3
#define LCD_BACKLIGHT_PIN  2

void lcd_init(void);

void lcd_data_start(void);
void lcd_data_stop(void);

void lcd_set_page_column(int page, int col);

void lcd_sleep(void);
void lcd_wakeup(void);

#endif // LCD_H
