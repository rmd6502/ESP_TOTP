#include "menuize.h"

static void itemToOffset(uint16_t item, uint8_t columns, uint16_t &x, uint16_t &y) {
  x = item % columns;
  y = item / columns;
}

Menu::Menu(std::vector<std::string> &items, TFT_eSPI &tft, uint8_t numColumns) : 
  mSelectedItem(0), mItems(items), mLastSelected(-1), mMenuoffset(0), mTFT(&tft), mNumColumns(numColumns) {
  mScreenHeight = mTFT->height() / mTFT->fontHeight() - 2;
}

void Menu::setItems(std::vector<std::string> &items, uint8_t numColumns) {
  mItems = items;
  mNumColumns = numColumns;
  mSelectedItem = 0;
  mLastSelected = -1;
  mMenuoffset = 0;
}

void Menu::selectItem(uint16_t newItem) {
  mSelectedItem = newItem;
}

void Menu::loop() {
  if (mLastSelected != mSelectedItem) {
    menuize();
  }
}

void Menu::menuize() {
  bool offsetChanged = false;
  mSelectedItem = max(0, min((int)(mItems.size() - 1), (int)mSelectedItem));
//  Serial.print("font height "); Serial.println(mTFT->fontHeight());
//  Serial.print("mSelectedItem "); Serial.println(mSelectedItem);
//  Serial.print("mMenuoffset "); Serial.println(mMenuoffset);
//  Serial.print("screenHeight "); Serial.println(mScreenHeight);
//  Serial.println();
  uint16_t row = 0, col = 0;

  itemToOffset(mSelectedItem, mNumColumns, col, row);
  if (row < mMenuoffset) {
    mMenuoffset = row;
    offsetChanged = true;
//    Serial.print("menuoffset up to ");Serial.println(mMenuoffset);
  } else if (row - mMenuoffset >= mScreenHeight) {
    mMenuoffset = row - mScreenHeight + 1;
    offsetChanged = true;
//    Serial.print("menuoffset down to ");Serial.println(mMenuoffset);
  }
  
  uint16_t colWidth = mTFT->width() / mNumColumns;

  if (offsetChanged || mLastSelected == -1) {
    uint16_t itemOffset = mMenuoffset * mNumColumns;
    for (int i=0; i < min((int)mScreenHeight * mNumColumns, (int)mItems.size() - itemOffset); ++i) {
      itemToOffset(i, mNumColumns, col, row);
      if (i + itemOffset == mSelectedItem) {
        mTFT->setTextColor(TFT_BLACK, TFT_PINK);
      } else {
        mTFT->setTextColor(TFT_GREEN, TFT_BLACK);
      }
      mTFT->drawString(mItems[i + itemOffset].c_str(), col * colWidth, row * mTFT->fontHeight() + 8);
    }
    offsetChanged = false;
  } else {
    itemToOffset(mSelectedItem, mNumColumns, col, row);
    mTFT->setTextColor(TFT_BLACK, TFT_PINK);
    mTFT->drawString(mItems[mSelectedItem].c_str(),col * colWidth,(row - mMenuoffset) * mTFT->fontHeight() + 8);
    
    uint16_t lastRow = 0, lastCol = 0;
    itemToOffset(mLastSelected, mNumColumns, lastCol, lastRow);
    if (mLastSelected > -1 && mLastSelected != mSelectedItem && lastRow >= mMenuoffset && lastRow - mMenuoffset <= mScreenHeight) {
      mTFT->setTextColor(TFT_GREEN, TFT_BLACK);
      mTFT->drawString(mItems[mLastSelected].c_str(),lastCol * colWidth,(lastRow - mMenuoffset) * mTFT->fontHeight() + 8);
    }
  }
  mLastSelected = mSelectedItem;
}
