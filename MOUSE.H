#if !defined(__MOUSE_H)
#define __MOUSE_H

#include "gui.h"

typedef void(interrupt far (*mousehook))();
typedef enum { mbLeft = 1, mbRight = 2, mbMiddle = 4 } MouseButton;

//鼠标位置，已映射为文本的行列号
struct MousePos {
    byte x, y;
};

struct MouseInfo {
    //鼠标位置，已映射为文本的行列号
    byte x, y;
    //按钮状态(bit)，位0=左键，位1=右键，位2中键
    byte button;
};

struct SmallRect { byte left, top, right, bottom; };

//鼠标是否可用
static bool MouseCanUse = false;

//初始化鼠标、显示鼠标光标
bool InitMouse();

//显示鼠标指针
void ShowMouse();

//隐藏鼠标指针
void HideMouse();

//获取鼠标坐标和按钮状态(坐标已映射为文本的行列号)
MouseInfo GetMouseInfo();

//设置鼠标坐标
void SetMousePos(int x, int y);

//鼠标坐标是否位于矩形区域内
byte MouseInRect(MouseInfo mi, SmallRect rect);

//设置鼠标隐藏区域
//void SetMouseHiddenArea(SmallRect rect);

//挂接鼠标中断响应程序
void SetMouseHook(mousehook hook);

void RemoveMouseHook();

#endif