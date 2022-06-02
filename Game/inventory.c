#include "input.h"
#include "inventory.h"
#include "scalar.h"
#include "window_map.h"

const char ItemNames[256][8] = {
    [ITEM_POTION] = "POTION"
};

window_task DisplayWindowTask = {
    .Type = TT_DISPLAY
};

item DisplayItem = {
    .Count = 1
};

inventory *Inventory;
inventory_state InventoryState;

inventory RedPC = {
    .WindowTask.Type = TT_INVENTORY,
    .ItemCapacity = 50,
    .Items = (item[50]) {{ITEM_POTION, 99}, {ITEM_POTION, 50}},
    .ItemCount = 2 
};

inventory Bag = {
    .WindowTask.Type = TT_INVENTORY,
    .ItemCapacity = 20,
    .Items = (item[20]) {{0}}
};

item RemoveItem(inventory *Inventory, int TossCount) {
    item *SelectedItem = &Inventory->Items[Inventory->ItemSelect];
    item RetItem = *SelectedItem;
    if(TossCount < SelectedItem->Count) { 
        SelectedItem->Count -= TossCount;
        RetItem.Count = TossCount;
    } else {
        Inventory->ItemCount--;
        for(int I = Inventory->ItemSelect; I < Inventory->ItemCount; I++) {
            Inventory->Items[I] = Inventory->Items[I + 1]; 
        }
        Inventory->ItemSelect = MinInt(Inventory->ItemSelect, Inventory->ItemCount);
    }
    return RetItem;
}

static item *FindItem(const inventory *Inventory, item_id ID, int *I) {
    I = I ? : &((int) {0});
    for(; *I < Inventory->ItemCount; ++*I) {
        if(Inventory->Items[*I].ID == ID) { 
            return &Inventory->Items[(*I)++];
        }
    }
    return NULL;
}

void AddItem(inventory *Inventory, item Item) {
    item *ExistingItem;
    int I = 0;
    while(Item.Count > 0 && (ExistingItem = FindItem(Inventory, Item.ID, &I))) {
        int ToAdd = MinInt(99 - ExistingItem->Count, Item.Count);
        ExistingItem->Count += ToAdd;
        Item.Count -= ToAdd; 
    } 
    if(Item.Count > 0 && Inventory->ItemCount < Inventory->ItemCapacity) {
        Inventory->Items[Inventory->ItemCount++] = Item; 
    }
}

void MoveItem(inventory *Dest, inventory *Src, int ItemCount) {
    if(Dest->ItemCount < Dest->ItemCapacity) {
        AddItem(Dest, RemoveItem(Src, ItemCount));
    }
} 

int PlaceItemCount(const inventory *Inventory, int ItemCount) {
    ItemCount = PosIntMod(ItemCount - 1, Inventory->Items[Inventory->ItemSelect].Count) + 1; 
    PlaceTextF((point) {16, 10}, "*%02d", ItemCount);
    return ItemCount;
}

void StartDisplayItem(inventory *Inventory) {
    PlaceTextBox((rect) {15, 9, 20, 12});
    PlaceItemCount(Inventory, DisplayItem.Count);
}

void UpdateDisplayItem(const inventory *Inventory) {
    if(VirtButtons[BT_UP] == 1) {
        DisplayItem.Count = PlaceItemCount(Inventory, DisplayItem.Count + 1); 
    } else if(VirtButtons[BT_DOWN] == 1) {
        DisplayItem.Count = PlaceItemCount(Inventory, DisplayItem.Count - 1); 
    } 
} 
