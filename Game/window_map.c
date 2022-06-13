#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "window_map.h"
#include "scalar.h"
#include "str.h"
#include "input.h"

const rect BottomTextRect = {0, 12, 20, 18};

uint8_t WindowMap[32][32];

menu MainMenu = { 
    .WindowTask.Type = TT_MENU,
    .Text = "NEW GAME\nOPTION",
    .Rect = {0, 0, 15, 6},
    .EndI = 2,
    .TextY = 2,
    .Flags = MF_AUTO_RESET
};

menu ContinueMenu = {
    .WindowTask.Type = TT_MENU,
    .Rect = {0, 0, 15, 8},
    .Text = "CONTINUE\nNEW GAME\nOPTION",
    .EndI = 3,
    .TextY = 2,
    .Flags = MF_AUTO_RESET
}; 

menu StartMenu = {
    .WindowTask.Type = TT_MENU,
    .Text = "POKéMON\nITEM\nRED\nSAVE\nOPTION\nEXIT", 
    .Rect = {10, 0, 20, 14},
    .TextY = 2,
    .EndI = 6,
};

menu RedPCMenu = {
    .WindowTask.Type = TT_MENU,
    .Text = "WITHDRAW ITEM\nDEPOSIT ITEM\nTOSS ITEM\nLOG OFF",
    .Rect = {0, 0, 16, 10},
    .TextY = 2,
    .EndI = 4,
    .Flags = MF_AUTO_RESET
};

menu YesNoMenu = {
    .WindowTask.Type = TT_MENU,
    .Text = "YES\nNO",
    .Rect = {0, 7, 6, 12},
    .TextY = 8,
    .EndI = 2,
    .Flags = MF_AUTO_RESET
};

menu UseTossMenu = {
    .WindowTask.Type = TT_MENU,
    .Text = "USE\nTOSS",
    .Rect = {13, 10, 20, 15},
    .TextY = 11,
    .EndI = 2,
    .Flags = MF_AUTO_RESET
};

menu ConfirmTossMenu = {
    .WindowTask.Type = TT_MENU,
    .Text = "YES\nNO",
    .Rect = {14, 7, 20, 12},
    .TextY = 8,
    .EndI = 2,
    .Flags = MF_AUTO_RESET
};

active_text ActiveText;

window_task *DeferedTask;
const char *DeferedMessage;

void PlaceTextBox(rect Rect) {
    /*HeadRow*/
    WindowMap[Rect.Top][Rect.Left] = MT_TOP_LEFT;
    memset(&WindowMap[Rect.Top][Rect.Left + 1], MT_MIDDLE, Rect.Right - Rect.Left - 2);
    WindowMap[Rect.Top][Rect.Right - 1] = MT_TOP_RIGHT;

    /*BodyRows*/
    for(int Y = Rect.Top + 1; Y < Rect.Bottom - 1; Y++) {
        WindowMap[Y][Rect.Left] = MT_CENTER_LEFT;
        memset(&WindowMap[Y][Rect.Left + 1], MT_BLANK, Rect.Right - Rect.Left - 2);
        WindowMap[Y][Rect.Right - 1] = MT_CENTER_RIGHT;
    }

    /*FootRow*/
    WindowMap[Rect.Bottom - 1][Rect.Left] = MT_BOTTOM_LEFT;
    memset(&WindowMap[Rect.Bottom - 1][Rect.Left + 1], MT_MIDDLE, Rect.Right - Rect.Left - 2);
    WindowMap[Rect.Bottom - 1][Rect.Right - 1] = MT_BOTTOM_RIGHT;
}

int CharToTile(const char **OutStr) {
    const char *Str = *OutStr;
    int Output = MT_BLANK;
    if(*Str == '*') {
        Output = MT_TIMES;
    } else if(*Str == '.') {
        Output = MT_PERIOD;
    } else if(*Str >= '0' && *Str <= ':') {
        Output = *Str + MT_ZERO - '0';
    } else if(*Str >= 'A' && *Str <= 'Z') {
        Output = *Str + MT_CAPITAL_A - 'A';
    } else if(*Str >= 'a' && *Str <= 'z') {
        Output = *Str + MT_LOWERCASE_A - 'a';
    } else if(*Str == '\xE9') {
        Output = MT_ACCENTED_E;
    } else if(*Str == '!') {
        Output = MT_EXCLAMATION_POINT;
    } else if(*Str == '\'') {
        Output = MT_SLASH;
    } else if(*Str == '-') {
        Output = MT_DASH;
    } else if(*Str == '~') {
        Output = MT_TIDLE;
    } else if(*Str == ',') {
        Output = MT_COMMA; 
    } else if(*Str == '?') {
        Output = MT_QUESTION; 
    } else {
        Output = MT_BLANK;
    } 
    (*OutStr)++;
    if(DoesStartStringMatch(Str, "é")) {
        Output = MT_ACCENTED_E; 
        (*OutStr)++;
    }
    return Output;
}

void PlaceText(point TileMin, const char *Text) {
    int X = TileMin.X;
    int Y = TileMin.Y;
    while(*Text) {
        switch(*Text) {
        case '\n':
            X = TileMin.X;
            Y += 2;
            Text++;
            break;
        case '\r':
            X = TileMin.X;
            Y += 3; 
            Text++;
            break;
        default:
            WindowMap[Y][X] = CharToTile(&Text);
            X++;
        }
    }
}

void PlaceTextF(point TileMin, const char *Format, ...) {
    char Text[256];
    va_list Args;
    va_start(Args, Format);
    vsnprintf(Text, sizeof(Text), Format, Args);
    va_end(Args);
    PlaceText(TileMin, Text);
}

void PlaceStaticText(rect Rect, const char *Text) {
    PlaceTextBox(Rect);
    PlaceText((point) {1, 14}, Text);
}

void PlaceMenuCursor(const menu *Menu, int MenuTile) {
    WindowMap[Menu->SelectI * 2 + Menu->TextY][Menu->Rect.Left + 1] = MenuTile; 
}

void PlaceMenu(const menu *Menu) {
    PlaceTextBox(Menu->Rect); 
    PlaceText((point) {Menu->Rect.Left + 2, Menu->TextY}, Menu->Text); 
    PlaceMenuCursor(Menu, MT_FULL_HORZ_ARROW);
}

void PlaceInventory(const inventory *Inventory) {
    PlaceTextBox((rect) {4, 2, 20, 13});
    int MinItem = Inventory->ItemSelect - Inventory->Y;
    for(int I = 0; I < 4; I++) {
        int ItemI = MinItem + I;
        int Y = I * 2 + 4;

        /*PlaceItem*/
        if(ItemI < Inventory->ItemCount) {
            PlaceText((point) {6, Y}, ItemNames[Inventory->Items[ItemI].ID]);
            PlaceTextF((point) {14, Y + 1}, "*%2d", Inventory->Items[ItemI].Count);
        } else if(ItemI == Inventory->ItemCount) {
            PlaceText((point) {6, Y}, "CANCEL");
        }
    }
    
    /*PlaceInventoryCursor*/
    WindowMap[Inventory->Y * 2 + 4][5] = MT_FULL_HORZ_ARROW;
}

void ClearWindow(void) {
    memset(WindowMap, 0, sizeof(WindowMap));
}

void ClearWindowRect(rect Rect) {
    for(int Y = Rect.Top; Y < Rect.Bottom; Y++) {
        for(int X = Rect.Left; X < Rect.Right; X++) {
            WindowMap[Y][X] = 0;
        }
    }
}

void MoveMenuCursor(menu *Menu) {
    if(VirtButtons[BT_UP] == 1 && Menu->SelectI > 0) {
        PlaceMenuCursor(Menu, MT_BLANK);
        Menu->SelectI--; 
        PlaceMenuCursor(Menu, MT_FULL_HORZ_ARROW);
    } else if(VirtButtons[BT_DOWN] == 1 && Menu->SelectI < Menu->EndI - 1) {
        PlaceMenuCursor(Menu, MT_BLANK);
        Menu->SelectI++; 
        PlaceMenuCursor(Menu, MT_FULL_HORZ_ARROW);
    }
}

void MoveMenuCursorWrap(menu *Menu) { 
    if(VirtButtons[BT_UP] == 1) {
        PlaceMenuCursor(Menu, MT_BLANK);
        Menu->SelectI = PosIntMod(Menu->SelectI - 1, Menu->EndI); 
        PlaceMenuCursor(Menu, MT_FULL_HORZ_ARROW);
    } else if(VirtButtons[BT_DOWN] == 1) {
        PlaceMenuCursor(Menu, MT_BLANK);
        Menu->SelectI = PosIntMod(Menu->SelectI + 1, Menu->EndI); 
        PlaceMenuCursor(Menu, MT_FULL_HORZ_ARROW);
    }
}


void FlashTextCursor(active_text *ActiveText) {
    WindowMap[16][18] = ActiveText->Tick++ / 30 % 2 ? MT_BLANK : MT_FULL_VERT_ARROW;
}

void ClearBottomWindow(void) { 
    ClearWindowRect(BottomTextRect);
}
