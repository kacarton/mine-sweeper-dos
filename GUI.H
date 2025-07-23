#if !defined(__GUI_H)
#define __GUI_H

#define byte unsigned char
typedef enum { false, true } bool;
//屏幕宽和高
#define SCREEN_WIDTH 80
#define SCREEN_HEIGHT 25
//标题颜色高亮蓝底白字
#define TITLE_COLOR ((LIGHTBLUE<<4) | WHITE)
//背景颜色
#define BACK_COLOR (DARKGRAY<<4)
//状态栏颜色
#define STATUSBAR_COLOR (LIGHTGRAY<<4 | YELLOW)
//对话框标题颜色
#define MENU_TITLE_COLOR (WHITE<<4 | BLACK)
#define MENU_COLOR (LIGHTBLUE<<4 | WHITE)
#define MENU_COLOR_HOT (BLUE<<4 | YELLOW)
#define SHADOW_COLOR (0|DARKGRAY)
//地雷网格颜色
#define GRID_TITLE_COLOR (LIGHTCYAN<<4)
#define GRID_BGCOLOR (LIGHTGRAY<<4 | DARKGRAY)
#define GRID_HOVER_COLOR (YELLOW<<4)
#define GRID_MINE_COLOR RED		//地雷颜色
#define GRID_DIGIT_COLOR BLUE	//数字颜色
#define GRID_FLAG_COLOR BLACK	//旗标颜色

#define VGAMemory ((char far*)0xB8000000L)

//设置显示模式为80*25 16色文本模式，顺带清屏
void SetTextMode();

//禁止背景闪烁，改为高亮背景色
void DisableBlink();

//隐藏光标
void HideCursor();
//显示光标
void ShowCursor();

//将字模写入显卡的字符发生器
void InstallChars();

/*清空屏幕指定区域的内容
  由left, top, right, bottom指定要清屏的矩形区域
  backcolor：空白区域的缺省颜色属性
*/
void ClearScreen(byte left, byte top, byte right, byte bottom, byte color);

/*改变屏幕区域的显示颜色
  由left, top, right, bottom指定要更改的矩形区域
  color：区域的新颜色属性
*/
void ChangeColor(byte x, byte y, byte color);
void ChangeColors(byte left, byte top, byte right, byte bottom, byte color);

/*改变单个字符的颜色
  由x, y:屏幕坐标位置
  color：字符颜色（背景色不变）
*/
void ChangeFontColor(byte x, byte y, byte color);

/*改变单个字符的背景颜色
  由x, y:屏幕坐标位置
  color：背景颜色（字符色不变）
*/
void ChangeBGColor(byte x, byte y, byte color);

/*以直接写显示内存方式输出文字
  x, y:输出字符的起始位置
  text:要显示的文字
  len:指定输出文字的数量
*/
void PrintText(byte x, byte y, char* text);
void PrintTextLen(byte x, byte y, char* text, int len);

/*以直接写显示内存方式重复输出字符
  x, y:输出字符的起始位置
  ch:要显示的字符
  count:重复次数
*/
void PrintChar(byte x, byte y, char ch, byte count);

#endif