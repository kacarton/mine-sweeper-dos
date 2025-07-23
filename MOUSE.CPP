#include <dos.h>
#include "mouse.h"

bool mouseshowed = false;

//初始化鼠标
bool InitMouse()
{
    //功能号0：初始化鼠标
    _AX = 0;
    geninterrupt(0x33);
    //出参AX＝0 不支持鼠标功能，0xFFFF 支持鼠标功能
    mouseshowed = false;
    MouseCanUse = (bool)_AX;
    return MouseCanUse;
}

//显示鼠标指针
void ShowMouse()
{
    if (!MouseCanUse || mouseshowed) return;
    _AX = 1;
    geninterrupt(0x33);
    mouseshowed = true;
}

//隐藏鼠标指针
void HideMouse()
{
    if (!MouseCanUse || !mouseshowed) return;
    _AX = 2;
    geninterrupt(0x33);
    mouseshowed = false;
}

//获取鼠标坐标和按钮状态(坐标已映射为文本的行列号)
MouseInfo GetMouseInfo()
{
    MouseInfo mi = {0, 0, 0};
    if (!MouseCanUse) return mi;
    _AX = 3;
    geninterrupt(0x33);
    mi.x = (byte)(_CX / 8);
    mi.y = (byte)(_DX / 8);
    mi.button = (byte)_BL;
    return mi;
}

//设置鼠标坐标
void SetMousePos(byte x, byte y)
{
    if (!MouseCanUse) return;
    _AX = 4;
    _CX = x * 8;
    _DX = y * 16;
    geninterrupt(0x33);
}

//鼠标坐标是否位于矩形区域内
byte MouseInRect(MouseInfo mi, SmallRect rect)
{
    return mi.x >= rect.left && mi.x <= rect.right
        && mi.y >= rect.top && mi.y <= rect.bottom;
}

/*/设置鼠标隐藏区域
//经测试不起作用，需自行实现此效果
void SetMouseHiddenArea(SmallRect rect)
{
    if (!MouseCanUse) return;
    _AX = 0x10;
    _CX = rect.left * 8;
    _DX = rect.top * 8;
    _SI = rect.right * 8;
    _DI = rect.bottom * 8;
    geninterrupt(0x33);
}*/


//挂接鼠标中断响应程序
void SetMouseHook(mousehook hook)
{
    if (!MouseCanUse) return;
    _AX = 0xC;
    _CX = 0x1F;//位置变化 + 左右按下/释放。忽略中间按钮
    _DX = FP_OFF(hook);
    _ES = FP_SEG(hook);
    geninterrupt(0x33);
}

void RemoveMouseHook()
{
    if (!MouseCanUse) return;
    _AX = 0xC;
    _CX = _ES = _DX = 0;
    geninterrupt(0x33);
}