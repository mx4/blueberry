#ifndef __LCD_H__
#define __LCD_H__

int LCD_SerialSetup(const char *devPath);
int LCD_WriteData(int fd, const char *str);
void LCD_SetVerbose(int v);

#endif /* __LCD_H__ */
