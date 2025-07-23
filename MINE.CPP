#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <math.h>
#include <time.h>
#include <dos.h>
#include "gui.h"
#include "mouse.h"

/**********************************************
说明：本游戏使用TurboC开发，在DOS下编译和运行。

更新记录：
2025-7-10：首个版本完成。
    使用两个半宽度字符+1个全宽字符显示正方网格，以白色和灰色相间实现类似棋盘外观的扫雷游戏。
    已实现键盘操作，可正常游戏。
2025-7-15：
    通过自定义字符显示接近于的网格形状，显示效果与windows版本的游戏更相似。
    支持鼠标操作，左击点开格子，右键标记，左键+右键快速开启九宫格。

Mine Sweeper (DOS) is released under the terms of the GNU General Public License.
Please see LICENSE.txt for the complete legal text.
***********************************************/

//主窗口标题
char* Title = "Mine Sweeper";
//状态栏提示文字
char* MainHint = " \x18\x19 Select, [Enter] Play, [Esc] Quit.";
char* AboutHint = "[F1] About ";
char* PlayingHint = " \x1B\x18\x19\x1A Move, [Space] Open field, [Enter] Mark.";
//主菜单标题
char* MainMenuTitle = "Level";
//主菜单文字
char* MainMenu[5] = {
        "Easy \t9*9 (10 mines)",
        "Normal \t16*16 (40 mines)",
        "Hard \t20*18 (72 mines)",
        "-",
        "Exit"
};

char* AboutContent[] = {
        "",
        "Thank you for playing Mine Sweeper (DOS).",
        "version 1.0 (2025-7-15)",
        "source code is open-sourced on GitHub and Gitee:",
        "\3\4https://github.com/kacarton/mine-sweeper-dos",
        "\3\4https://gitee.com/kacarton/mine-sweeper-dos",
        "Feedback via email:",
        "\1\x2peng(kacarton@msn.com)",
        ""
};

//三个难度级别的地雷数量
byte MinesOfLevel[] = {10, 40, 72};
short MineCounter = 0;
//当前选择的难度级别
byte Level = 0;

//网格大小
struct {
    //左上角坐标
    byte left, top;
    //网格大小
    byte width, height;
} GridSize;

//当前坐标
struct {
    byte x, y;
} Position;

//地雷状态
typedef enum {
    COVERED,       //未翻开
    FLAGGED,       //标识为地雷
    QUESTIONED,    //标识为存疑
    OPENED         //已翻开，如果是地雷表示已炸
} MineState;

//地雷数组
struct MINE {
    //9宫格内的地雷数量，0xF表示自身为地雷
    byte mines:4;
    //是否已开启
    MineState state:4;
} Mines[24*18]; //max value is 20*18

                     //空白  //标识为地雷  存疑         已翻开已炸
char* MineChars[] = { "  ", "\xFB\xFC", "\xA7\xA8", "\xE\xF"};

clock_t start_time;

/*为指定区域绘制阴影
  left, top, right, bottom：区域的四个角的坐标值
  BGColor: 该区域背景色，正下方用半个字符填充，需要使用到背景色
*/
void DrawShadow(byte left, byte top, byte right, byte bottom, byte BGColor)
{
    //标题右则阴影
    PrintChar(right, top, 0xDF, 1);
    //菜单下方阴影
    PrintChar(left, bottom, 0xDF, right - left);
    //填充菜单阴影
    ChangeColors(right, top, right, bottom, SHADOW_COLOR);
    ChangeColor(left, bottom, BACK_COLOR & 0xF0 | BGColor>>4 & 0xF);
    ChangeColors(left + 1, bottom, right, bottom, SHADOW_COLOR & 0xF0 | BGColor>>4 & 0xF);
}

//等待Esc钮或鼠标点击
void WaitForKey()
{
    MouseInfo mi = GetMouseInfo();
    bool leftBtnDown = (bool)(mi.button & mbLeft);
    while (1)
    {
        if (kbhit())
        {
            char key = getch();
            //按Esc、空格、回车均可返回
            if (27 == key || 32 == key || 13 == key)
                break;
        }

        //监测鼠标左键，按下→弹出时退出
        mi = GetMouseInfo();
        if (leftBtnDown != (bool)(mi.button & mbLeft))
        {
            leftBtnDown = (bool)(mi.button & mbLeft);
            if (!leftBtnDown)
                break;
        }
        delay(1);
    }
}

void ShowAbout()
{
    HideMouse();
    byte size = sizeof(AboutContent) / sizeof(AboutContent[0]);
    byte len = 0;
    for (byte i = 0; i < size; i++)
    {
        if (strlen(AboutContent[i]) > len)
            len = strlen(AboutContent[i]);
    }

    byte x = (SCREEN_WIDTH - len) / 2 - 1;
    byte y = (SCREEN_HEIGHT - size) / 2 - 1;
    //菜单标题颜色
    ClearScreen(x, y, x + len + 2, y, MENU_TITLE_COLOR);
    //菜单颜色
    ClearScreen(x, y + 1, x + len + 1, y + size, MENU_COLOR);
    DrawShadow(x, y, x + len + 2, y + size + 1, MENU_COLOR);

    //居中输出标题
    char s[20];
    sprintf(s, "%s %s", "About", Title);
    PrintText(x + (len - strlen(s)) / 2, y, s);
    for (i = 0; i < size; i++)
    {
        //带图标的文字用黄色字显示
        if (AboutContent[i][0] < 32)
        {
            PrintText(x + 2, y + i + 1, AboutContent[i]);
            ChangeColors(x + 1, y + i + 1, x + len, y + i + 1, MENU_COLOR & 0xF0 | YELLOW);
        }
        else
            PrintText(x + 1, y + i + 1, AboutContent[i]);
    }
    ShowMouse();
    WaitForKey();
    HideMouse();
    //退出时清屏
    ClearScreen(0, 1, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 2, BACK_COLOR);
}

/*在指定位置绘制菜单
  left, top：显示位置
  title：菜单标题
  menus：菜单内容
  count: 菜单项目数
  index：当前选择项
*/
byte DrawMenu(byte left, byte top, char* title, char* menus[], byte count, byte index)
{
    //计算菜单文字的长度
    byte len = 0, hintX = 0, speration = count;
    for(byte i=0; i<count; i++)
    {
        if ('-' == menus[i][0])
           speration = i;
        else if (strlen(menus[i]) > len)
           len = strlen(menus[i]);
        char* pos = strchr(menus[i], '\t');
        if (NULL != pos && pos - menus[i] + 2 > hintX)
           hintX = pos - menus[i] + 2;
    }
    //文字边距1个空格
    len += 3;

    byte x = left, y = top;
    //如果x和y都是0xFF表示居中显示，x、y值需要计算
    if (0xFF == x && 0xFF == y)
    {
        x = (SCREEN_WIDTH - len) / 2;
        y = (SCREEN_HEIGHT - count - 1) / 2 - 1;
    }
DrawMenu_Start:
    //菜单标题颜色
    ClearScreen(x, y, x + len, y, MENU_TITLE_COLOR);
    //菜单颜色
    ClearScreen(x, y + 1, x + len + 1, y + count, MENU_COLOR);
    ClearScreen(x + hintX + 1, y + 1, x + len, y + speration, MENU_COLOR & 0xF0 | LIGHTGRAY);

    DrawShadow(x, y, x + len + 1, y + count + 1, MENU_COLOR);

    //居中输出标题
    PrintText(x + (len - strlen(title)) / 2 , y, title);
    //输出菜单文字
    for(i=0; i<count; i++)
    {
        //如果是字符-，表示画分隔线
        if ('-' == menus[i][0])
            PrintChar(x + 1, y + i + 1, 0xC4, len - 1);
        else
        {
            char* pos = strchr(menus[i], '\t');
            if (NULL == pos)
               PrintText(x + 2, y + i + 1, menus[i]);
            else
            {
                PrintTextLen(x + 2, y + i + 1, menus[i], pos - menus[i]);
                PrintText(x + len - strlen(pos+1) - 1, y + i + 1, pos+1);
            }
        }
    }
    //按选中项改变颜色
    ChangeColors(x + 1, y + 1 + index, x + len - 1, y + 1 + index, MENU_COLOR_HOT);

    //鼠标位置
    MousePos mPos = { 0, 0 };
    //左键状态，用于后面检测按钮变化
    bool leftBtnDown = false;
    MouseInfo mi;
    if (MouseCanUse) {
       ShowMouse();
       mi = GetMouseInfo();
       mPos.x = mi.x;
       mPos.y = mi.y;
       leftBtnDown = (bool)(mi.button & mbLeft);
    }

    //接收按钮，上下选择菜单项，回车确认选择，Esc键退出
    while(1)
    {
        //检测按钮
        if (kbhit())
        switch(getch())
        {
            case 27: //Esc
                //if (MouseCanUse)
                HideMouse();
                return 0xFF;
            case 13: //Enter
                //if (MouseCanUse)
                HideMouse();
                return index;
            case 0x48:
                 ChangeColors(x + 1, y + 1 + index, x + len - 1, y + 1 + index, MENU_COLOR);
                 if (NULL != strchr(menus[index], '\t'))
                     ChangeColors(x + hintX + 1, y + 1 + index, x + len, y + 1 + index, MENU_COLOR & 0xF0 | LIGHTGRAY);
                 if (0 == index)
                     index = count - 1;
                 else
                 {
                     index--;
                     if ('-' == menus[index][0])
                        index--;
                 }
                 ChangeColors(x + 1, y + 1 + index, x + len - 1, y + 1 + index, MENU_COLOR_HOT);
                 break;
            case 0x50:
                 ChangeColors(x + 1, y + 1 + index, x + len - 1, y + 1 + index, MENU_COLOR);
                 if (NULL != strchr(menus[index], '\t'))
                     ChangeColors(x + hintX + 1, y + 1 + index, x + len, y + 1 + index, MENU_COLOR & 0xF0 | LIGHTGRAY);
                 index++;
                 if ('-' == menus[index][0])
                    index++;
                 if (index == count)
                    index = 0;
                 ChangeColors(x + 1, y + 1 + index, x + len - 1, y + 1 + index, MENU_COLOR_HOT);
                 break;
            case 0x3B: //F1
                ShowAbout();
                goto DrawMenu_Start;
        }

        //检查鼠标位置，更新选中项。按下左键确认选择，返回
        if (MouseCanUse)
        {
            mi = GetMouseInfo();
            SmallRect rect = { (byte)x + 1, (byte)y + 1, (byte)x + len - 1, (byte)y + count };

            //点击左上角退出
            if (0 == mi.y && mi.x >= 0 && mi.x <= 1 && mbLeft == (mi.button & mbLeft))
                return 0xFF;

            if (!MouseInRect(mi, rect)) continue;
            if (mPos.x != mi.x || mPos.y != mi.y)
            {
                mPos.x = mi.x;
                mPos.y = mi.y;
                byte newIndex = mi.y - y - 1;
                if (newIndex == index || '-' == menus[newIndex][0])
                    continue;
                //更新选中项颜色前先隐藏鼠标，避免鼠标自身处理光标色块时留下残像
                HideMouse();
                ChangeColors(x + 1, y + 1 + index, x + len - 1, y + 1 + index, MENU_COLOR);
                if (NULL != strchr(menus[index], '\t'))
                    ChangeColors(x + hintX + 1, y + 1 + index, x + len, y + 1 + index, MENU_COLOR & 0xF0 | LIGHTGRAY);
                index = newIndex;
                ChangeColors(x + 1, y + 1 + index, x + len - 1, y + 1 + index, MENU_COLOR_HOT);
                ShowMouse();
            }
            if (leftBtnDown != (mi.button & mbLeft))
            {
                leftBtnDown = (bool)(mi.button & mbLeft);
                //放开鼠标左键时确认选择，返回调用程序
                //bugfix:如果按下就返回HideMouse()后，下次再进入按钮状态依然是按下状态
                if (!leftBtnDown) {
                    HideMouse();
                    return index;
                }
            }
        }
    }
}

//绘制主窗口
byte ShowMainForm()
{
    //主标题颜色
    ClearScreen(0, 0, SCREEN_WIDTH - 1, 0, TITLE_COLOR);
    //主窗口背景颜色
    ClearScreen(0, 1, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 2, BACK_COLOR);
    //底部状态栏颜色
    ClearScreen(0, SCREEN_HEIGHT - 2, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 2,BACK_COLOR | LIGHTGRAY);
    ClearScreen(0, SCREEN_HEIGHT - 1, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, STATUSBAR_COLOR);
    ChangeColors(SCREEN_WIDTH - 24, SCREEN_HEIGHT - 1, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 1, STATUSBAR_COLOR & 0xF0);
    //输出主标题，前面打印黑色[]好看一点
    PrintText(0, 0, "\x11\x10");
    ChangeColors(0, 0, 1, 0, LIGHTGRAY<<4 & 0xF0);
    //PrintText(1, 0, "\xF0");
    //标题居中
    PrintText((SCREEN_WIDTH - strlen(Title)) / 2, 0, Title);
    //底部打印提示文字
    PrintText(0, SCREEN_HEIGHT - 1, MainHint);
    PrintText(SCREEN_WIDTH - strlen(AboutHint), SCREEN_HEIGHT - 1, AboutHint);
    //绘制主菜单
    byte index = DrawMenu(0xFF, 0xFF, MainMenuTitle, MainMenu, sizeof(MainMenu)/sizeof(MainMenu[0]), Level);
    //退出时清屏
    ClearScreen(0, 1, SCREEN_WIDTH - 1, SCREEN_HEIGHT - 2, BACK_COLOR);
    if (index < 3)
       Level = index;
    return index;
}

//在状态栏显示游戏时间、剩余地雷数量、坐标值
void UpdateStatusBar()
{
    clock_t now_time = clock();
    char s[30];
    sprintf(s, "     \xB3%ds, mines:%d \xB3%d,%d "
        , (int)((double)(now_time - start_time) / CLOCKS_PER_SEC)
        , MineCounter, Position.x + 1, Position.y + 1);
    PrintText(SCREEN_WIDTH - strlen(s), SCREEN_HEIGHT - 1, s);
}

//绘制网络
void DrawGrid(char* title)
{
    byte x = GridSize.left - 1;
    byte y = GridSize.top - 1;
    //清除内容，用指定颜色填充区域
    ClearScreen(x, y, x + GridSize.width * 2 + 1, y, GRID_TITLE_COLOR);
    ClearScreen(x, y + 1, x + GridSize.width * 2 + 1, y + GridSize.height, GRID_BGCOLOR);
    DrawShadow(x, y, x + GridSize.width * 2 + 2, y + GridSize.height + 1, GRID_BGCOLOR);

    //如果标题有\t字符，只显示\t前的内容
    //PrintChar(x + GridSize.width, y, '\1', 1);
    char* pos = strchr(title, '\t');
    if (NULL == pos)
       PrintText(x + GridSize.width - strlen(title) / 2 + 1, y, title);
    else
       PrintTextLen(x + GridSize.width - (pos - title) / 2 + 1, y, title, pos - title);

    //底部状态栏提示
    PrintText(0, SCREEN_HEIGHT - 1, PlayingHint);

    //绘制网格
    for(byte i = 0; i < GridSize.width; i++)
    {
        for (byte j = 1; j <= GridSize.height; j++)
            PrintText(x + i * 2 + 1, y + j, "\x80\x81");
    }
    //左则和右则补一条竖线
    for (i = 1; i <= GridSize.height; i++)
    {
        PrintChar(x, y + i, '\xB4', 1);
        PrintChar(x + GridSize.width * 2 + 1, y + i, '\xB3', 1);
    }
    //更改下方阴影，补上横线
    PrintChar(x + 1, y + 1 + GridSize.height, '\xDC', GridSize.width*2);
    ChangeColors(x + 1, y + 1 + GridSize.height, x + GridSize.width*2, y + 1 + GridSize.height, SHADOW_COLOR >>4 | GRID_BGCOLOR & 0xF0);
}

//以直接写屏方式显示当前坐标（黄色方块，键盘可移动）
void UpdatePosition(byte x, byte y)
{
    /*ChangeColors(Position.x * 2 + GridSize.left,
        Position.y + GridSize.top,
        Position.x * 2 + GridSize.left + 2,
        Position.y + GridSize.top,
        0 == x && 0 == (y & 1) ?  : GRID_COLOR
    );*/
    //旧坐标恢复灰色显示
    int base = ((Position.y + GridSize.top) * SCREEN_WIDTH + Position.x * 2 + GridSize.left) * 2 + 1;
    VGAMemory[base] = VGAMemory[base + 2] = VGAMemory[base] & 0xF | GRID_BGCOLOR & 0xF0;

    Position.x = x;
    Position.y = y;
    //新坐标以黄色显示
    base = ((y + GridSize.top) * SCREEN_WIDTH + x * 2 + GridSize.left) * 2 + 1;
    VGAMemory[base] = VGAMemory[base + 2] = VGAMemory[base] & 0xF | GRID_HOVER_COLOR;
}

//初始化地雷数量
void InitMines()
{
    byte total = MineCounter = MinesOfLevel[Level];
    //全部清0
    memset(Mines, 0, sizeof(Mines));
    srand(time(NULL));
    byte x, y;
    //随机生成指定数量的地雷
    while (total > 0)
    {
        x = rand() % GridSize.width;
        y = rand() % GridSize.height;
        if (0 == Mines[x * GridSize.width + y].mines)
        {
            Mines[x * GridSize.width + y].mines = 0xF;//0xF表示地雷
            total--;
        }
    }

    //计算每个单元格周围的地雷数量
    for (x = 0; x < GridSize.width; x++)
    {
        for (y = 0; y < GridSize.height; y++)
        {
            if (Mines[x * GridSize.width + y].mines == 0xF) //0xF表示地雷
                continue;

            //按九宫格（3*3）计算附近地雷数量
            int count = 0;
            for (int i = x - 1; i <= x + 1; i++)
            {
                for (int j = y - 1; j <= y + 1; j++)
                {
                    if (i >= 0 && i < GridSize.width
                       && j >= 0 && j < GridSize.height
                       && 0xF == Mines[i * GridSize.width + j].mines)
                    {
                        count++;
                    }
                }
            }
            Mines[x * GridSize.width + y].mines = count;
        }
    }

    /* for debug
    for(x=0; x<GridSize.width; x++)
    for(y=0; y<GridSize.height; y++)
    if (0 != Mines[x * GridSize.width + y].mines)
    {
        if (0xF == Mines[x * GridSize.width + y].mines)
        {
        PrintText(GridSize.left + x * 2, GridSize.top + y, MineChars[OPENED]);
        ChangeFontColor(GridSize.left + x * 2, GridSize.top + y, GRID_MINE_COLOR);
        ChangeFontColor(GridSize.left + x * 2+1, GridSize.top + y, GRID_MINE_COLOR);
        }
        else
        {
        PrintChar(GridSize.left + x * 2, GridSize.top + y, Mines[x * GridSize.width + y].mines * 2 + 0x80, 1);
        PrintChar(GridSize.left + x * 2 + 1, GridSize.top + y, Mines[x * GridSize.width + y].mines * 2 + 0x81, 1);
        ChangeFontColor(GridSize.left + x * 2, GridSize.top + y, GRID_DIGIT_COLOR);
        ChangeFontColor(GridSize.left + x * 2 + 1, GridSize.top + y, GRID_DIGIT_COLOR);
        }
    }//*/
}

void Boom()
{
    for (byte x = 0; x < GridSize.width; x++)
        for (byte y = 0; y < GridSize.height; y++)
        {
            MINE *m = &Mines[x * GridSize.width + y];
            if (0xF == m->mines)
            {
                if (FLAGGED != m->state)
                {
                    ChangeFontColor(GridSize.left + x * 2, GridSize.top + y, GRID_MINE_COLOR);
                    ChangeFontColor(GridSize.left + x * 2 + 1, GridSize.top + y, GRID_MINE_COLOR);
                }
                m->state = OPENED;
                PrintText(GridSize.left + x * 2, GridSize.top + y, MineChars[OPENED]);
            }
            else if (FLAGGED == m->state)
            {
                //PrintText(GridSize.left + x * 2, GridSize.top + y, "\xFD\xFE");
                ChangeFontColor(GridSize.left + x * 2, GridSize.top + y, GRID_MINE_COLOR);
                ChangeFontColor(GridSize.left + x * 2 + 1, GridSize.top + y, GRID_MINE_COLOR);
            }
        }
    ChangeColors(GridSize.left - 1, GridSize.top - 1, GridSize.left + GridSize.width * 2, GridSize.top - 1, GRID_HOVER_COLOR | GRID_MINE_COLOR);
    char* s = "You lose";
    PrintText(GridSize.left + GridSize.width - strlen(s) / 2, GridSize.top - 1, s);
    ClearScreen(0, SCREEN_HEIGHT - 1, SCREEN_WIDTH - 25, SCREEN_HEIGHT - 1, STATUSBAR_COLOR);
    PrintText(1, SCREEN_HEIGHT - 1, "Press [Esc] to quit.");
    //按Esc键或鼠标左键退出
    HideMouse();
    WaitForKey();
}

bool IsWin()
{
   for (byte x = 0; x < GridSize.width; x++)
        for (byte y = 0; y < GridSize.height; y++)
        {
            MINE m = Mines[x * GridSize.width + y];
            if (0xF == m.mines && m.state != FLAGGED)
            {
                return false;
            }
        }
    UpdateStatusBar();
    ClearScreen(0, SCREEN_HEIGHT - 1, SCREEN_WIDTH - 25, SCREEN_HEIGHT - 1, STATUSBAR_COLOR);
    PrintText(1, SCREEN_HEIGHT - 1, "Press [Esc] to quit.");
    char* s = "YOU WIN!";
    ClearScreen(GridSize.left - 1, GridSize.top - 1, GridSize.left + GridSize.width * 2, GridSize.top - 1, LIGHTGREEN<<4 | BLUE);
    PrintText(GridSize.left + GridSize.width - strlen(s) / 2, GridSize.top - 1, s);

    //按Esc键或鼠标左键退出
    HideMouse();
    WaitForKey();
    return true;
}

//递归翻开空白区域
void OpenMines(int x, int y)
{
    if (x < 0 || x >= GridSize.width || y < 0 || y >= GridSize.height
        || COVERED != Mines[x * GridSize.width + y].state)
    {
        return;
    }

    MINE *mine = &Mines[x * GridSize.width + y];
    mine->state = OPENED;
    PrintChar(GridSize.left + x * 2, GridSize.top + y
        , mine->mines ? mine->mines * 2 + 0x80 : ' ' //'\xF9'
        , 1);
    PrintChar(GridSize.left + x * 2 + 1, GridSize.top + y
        , mine->mines ? mine->mines * 2 + 0x81 : ' ' //'\xF9'
        , 1);
    if (mine->mines)
    {
        ChangeFontColor(GridSize.left + x * 2, GridSize.top + y, GRID_DIGIT_COLOR);
        ChangeFontColor(GridSize.left + x * 2 + 1, GridSize.top + y, GRID_DIGIT_COLOR);
    }
    //else
    //   ChangeColors(GridSize.left + x * 2, GridSize.top + y, 
    //       GridSize.left + x * 2 + 1, GridSize.top + y,
    //       DARKGRAY | DARKGRAY<<4);
    if (0 == mine->mines)
    {
        for (int i = x - 1; i <= x + 1; i++)
        {
            for (int j = y - 1; j <= y + 1; j++)
            {
                OpenMines(i, j);
            }
        }
    }
}

//按下数字翻开周边8个格子
void Open8Mines(int x, int y)
{
    if (x < 0 || x >= GridSize.width || y < 0 || y >= GridSize.height
        || OPENED != Mines[x * GridSize.width + y].state)
    {
        return;
    }

    //判断能不能翻开
    MINE *mine = &Mines[x * GridSize.width + y];
    byte flag = 0;
    for (int i = x - 1; i <= x + 1; i++)
    {
        for (int j = y - 1; j <= y + 1; j++)
        {
            if (i >= 0 && i < GridSize.width && j >= 0 && j < GridSize.height
                && FLAGGED == Mines[i * GridSize.width + j].state)
                flag++;
        }
    }
    if (flag != mine->mines)
        return;

    //翻开并检查是否遇雷
    bool boom = false;
    for (i = x - 1; i <= x + 1; i++)
    {
        for (int j = y - 1; j <= y + 1; j++)
        {
            mine = &Mines[i * GridSize.width + j];
            if (i >= 0 && i < GridSize.width && j >= 0 && j < GridSize.height
                && COVERED == Mines[i * GridSize.width + j].state)
            {
                mine->state = OPENED;
                PrintChar(GridSize.left + i * 2, GridSize.top + j,
                    mine->mines ? mine->mines * 2 + 0x80 : ' ', 1);
                PrintChar(GridSize.left + i * 2 + 1, GridSize.top + j,
                    mine->mines ? mine->mines * 2 + 0x81 : ' ', 1);
                if (mine->mines)
                {
                    ChangeFontColor(GridSize.left + i * 2, GridSize.top + j, GRID_DIGIT_COLOR);
                    ChangeFontColor(GridSize.left + i * 2 + 1, GridSize.top + j, GRID_DIGIT_COLOR);
                }
                if (0xF == mine->mines)
                    boom = true;
            }
        }
    }
    if (boom)
        Boom();
}

//鼠标处理程序

bool MouseHandler(MouseInfo* mInfo)
{
    MouseInfo mi = GetMouseInfo();
    //点击左上角退出
    if (0 == mi.y && mi.x >= 0 && mi.x <= 1 && mbLeft == (mi.button & mbLeft))
    {
        HideMouse();
        return false;
    }

    SmallRect rect = {(byte)(GridSize.left), (byte)(GridSize.top), (byte)(GridSize.left + GridSize.width * 2 - 1), (byte)(GridSize.top + GridSize.height - 1)};
    if (!MouseInRect(mi, rect))
    {
        ShowMouse();
        return true;
    }
    HideMouse();
    
    if (mInfo->x == mi.x && mInfo->y == mi.y && mInfo->button == mi.button)
        return true;
    if (mInfo->x != mi.x || mInfo->y != mi.y)
    {
        mInfo->x = mi.x; mInfo->y = mi.y;
        UpdatePosition((mi.x - GridSize.left) / 2, mi.y - GridSize.top);
    }
    if (mInfo->button != mi.button)
    {
        //同时按下鼠标左键和右键，之后释放
        if ((mbLeft | mbRight) == (mInfo->button & (mbLeft | mbRight))
            && 0 == (mi.button & (mbLeft | mbRight)))
        {
            //遇到数字，翻开周边8个格
            if (Mines[Position.x * GridSize.width + Position.y].state == OPENED
                && Mines[Position.x * GridSize.width + Position.y].mines > 0)
            {
                Open8Mines(Position.x, Position.y);
            }
        }
        //鼠标左键释放
        if ((mInfo->button & mbLeft) != (mi.button & mbLeft) && 0 == (mi.button & mbLeft))
        {
            //向键盘缓冲区写入空格键
            _AH = 5;       
            _CH = 0x39; //扫描码
            _CL = 0x20; //ASCII码
            geninterrupt(0x16);
        }
        //鼠标右键释放
        if ((mInfo->button & mbRight) != (mi.button & mbRight) && 0 == (mi.button & mbRight))
        {
            //向键盘缓冲区写入回车键
            _AH = 5;
            _CH = 0x1C; //扫描码
            _CL = 0x0D; //ASCII码
            geninterrupt(0x16);
        }

        mInfo->button = mi.button;
    }

    return true;
}

//响应按键事件
bool KeybordHanlder()
{
    switch (getch())
    {
    case 27: //ESC
        return false;
    case 0x48: //↑
        if (Position.y > 0)
            UpdatePosition(Position.x, Position.y - 1);
        break;
    case 0x50: //↓
        if (Position.y < GridSize.height - 1)
            UpdatePosition(Position.x, Position.y + 1);
        break;
    case 0x4B: //<-
        if (Position.x > 0)
            UpdatePosition(Position.x - 1, Position.y);
        break;
    case 0x4D: //->
        if (Position.x < GridSize.width - 1)
            UpdatePosition(Position.x + 1, Position.y);
        break;
        //按回车键切换标记状态
    case 13: //enter
        byte state = Mines[Position.x * GridSize.width + Position.y].state;
        //已翻开的不处理
        if (OPENED == state)
            return true;
        //标记为地雷切换到存疑，地雷数量+1
        if (FLAGGED == state)
            MineCounter++;
        state++;
        //未翻开切换到地雷标记，地雷数量-1
        if (FLAGGED == state)
            MineCounter--;
        if (OPENED == state)
            state = COVERED;
        Mines[Position.x * GridSize.width + Position.y].state = (MineState)state;
        if (COVERED == state)
            PrintText(GridSize.left + Position.x * 2, GridSize.top + Position.y, "\x80\x81");
        else
            PrintText(GridSize.left + Position.x * 2, GridSize.top + Position.y, MineChars[state]);
        byte fontcolor = GRID_BGCOLOR & 0xF;
        if (FLAGGED == state || QUESTIONED == state)
            fontcolor = GRID_FLAG_COLOR;
        ChangeFontColor(GridSize.left + Position.x * 2, GridSize.top + Position.y, fontcolor);
        ChangeFontColor(GridSize.left + Position.x * 2 + 1, GridSize.top + Position.y, fontcolor);
        if (0 == MineCounter && IsWin())
            return false;
        break;
        //按空格键翻开网格
    case 32: //space
        //碰到地雷爆炸
        if (0xF == Mines[Position.x * GridSize.width + Position.y].mines
            && Mines[Position.x * GridSize.width + Position.y].state == COVERED)
        {
            Boom();
            return false;
        }
        //遇到数字，翻开周边8个格
        if (Mines[Position.x * GridSize.width + Position.y].state == OPENED
            && Mines[Position.x * GridSize.width + Position.y].mines > 0)
        {
            Open8Mines(Position.x, Position.y);
        }
        OpenMines(Position.x, Position.y);
        break;
    }
    return true;
}

void Play(){
    //根据等级生成不同尺寸的网格
    switch(Level)
    {
        case 0:
             GridSize.width = GridSize.height = 9;
             Position.x = Position.y = 4;
             break;
        case 1:
             GridSize.width = GridSize.height = 16;
             Position.x = Position.y = 7;
             break;
        case 2:
             GridSize.width = 20;
             GridSize.height = 18;
             Position.x = 9;
             Position.y = 8;
             break;
    }
    GridSize.left = SCREEN_WIDTH / 2 - GridSize.width;
    GridSize.top = (SCREEN_HEIGHT - GridSize.height) / 2;
    DrawGrid(MainMenu[Level]);
    UpdatePosition(Position.x, Position.y);
    InitMines();
    ShowMouse();
    MouseInfo mi = {0, 0, 0};
    if (MouseCanUse)
    {
        MouseHandler(&mi);
    }
    //记录游戏时间
    start_time = clock();
    //响应按键和鼠标
    while(1)
    {
        //更新时间、地雷数量、坐标位置
        UpdateStatusBar();
        delay(0);
        //KeybordHanlder或MouseHandler返回false表示结束游戏
        if (MouseCanUse && !MouseHandler(&mi)) break;
        if (kbhit() && !KeybordHanlder()) break;
    }
    HideMouse();
}

void main(){
    SetTextMode();
    //禁用颜色闪烁
    DisableBlink();
    //隐藏光标
    HideCursor();
    InstallChars();
    MouseCanUse = (bool)InitMouse();
    byte MenuCount = sizeof(MainMenu) / sizeof(MainMenu[0]);
    while(1)
    {
        byte index = ShowMainForm();
        if (0xFF == index || index == MenuCount - 1)
           break;
        Play();
    }

    SetTextMode();
    MenuCount = sizeof(AboutContent) / sizeof(AboutContent[0]);
    for (byte i = 0; i < MenuCount; i++)
    {
        if (0 == AboutContent[i][0])
           continue;
        if (AboutContent[i][0] < 32)
            printf("  %s\r\n", &AboutContent[i][2]);
        else
            printf("%s\r\n", AboutContent[i]);
    }
}