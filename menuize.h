#include <string>
#include <vector>
#include <TFT_eSPI.h>

class Menu {
  public:
    Menu(std::vector<std::string> &items, TFT_eSPI &tft, uint8_t columns=1);
    void loop();
    void selectItem(uint16_t item);
    uint16_t selectedItem() { return mSelectedItem; }
  private:
    uint16_t mSelectedItem;
    std::vector<std::string> mItems;
    int16_t mLastSelected;
    uint16_t mMenuoffset;
    uint16_t mScreenHeight;
    TFT_eSPI *mTFT;
    uint8_t mNumColumns;
  
    void menuize();
};
