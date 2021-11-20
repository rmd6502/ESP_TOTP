#include "menuize.h"

Menu::Menu(std::vector<std::string> &items, TFT_eSPI &tft) : 
  mSelectedItem(0), mItems(items), mLastSelected(-1), mMenuoffset(0), mTFT(&tft) {
  mScreenHeight = mTFT->height() / mTFT->fontHeight() - 2;
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
  Serial.print("mSelectedItem "); Serial.println(mSelectedItem);
  Serial.print("mMenuoffset "); Serial.println(mMenuoffset);
  Serial.print("screenHeight "); Serial.println(mScreenHeight);
  Serial.println();
  
  if (mSelectedItem < mMenuoffset) {
    mMenuoffset = mSelectedItem;
    offsetChanged = true;
    Serial.print("menuoffset up to ");Serial.println(mMenuoffset);
  } else if (mSelectedItem - mMenuoffset >= mScreenHeight) {
    mMenuoffset = mSelectedItem - mScreenHeight + 1;
    offsetChanged = true;
    Serial.print("menuoffset down to ");Serial.println(mMenuoffset);
  }

  if (offsetChanged || mLastSelected == -1) {
    int mSelectedItementLine = mMenuoffset;
    for (int i=0; i < min((int)mScreenHeight, (int)mItems.size() - mMenuoffset); ++i) {
      if (mSelectedItementLine == mSelectedItem) {
        mTFT->setTextColor(TFT_BLACK, TFT_PINK);
      } else {
        mTFT->setTextColor(TFT_GREEN, TFT_BLACK);
      }
      mTFT->drawString(mItems[mSelectedItementLine++].c_str(), 0, i * mTFT->fontHeight() + 8);
    }
    offsetChanged = false;
  } else {
    //mTFT->fillRoundRect(0,(mSelectedItem - mMenuoffset) * mTFT->fontHeight(),mTFT->textWidth(mItems[mSelectedItem].c_str()),8,2,TFT_PINK);
    mTFT->setTextColor(TFT_BLACK, TFT_PINK);
    mTFT->drawString(mItems[mSelectedItem].c_str(),0,(mSelectedItem - mMenuoffset) * mTFT->fontHeight() + 8);
    if (mLastSelected > -1 && mLastSelected != mSelectedItem && mLastSelected >= mMenuoffset && mLastSelected - mMenuoffset <= mScreenHeight) {
      //mTFT->fillRoundRect(0,(mLastSelected - mMenuoffset) * mTFT->fontHeight(),mTFT->textWidth(mItems[mLastSelected].c_str()),8,2,TFT_BLACK);
      mTFT->setTextColor(TFT_GREEN, TFT_BLACK);
      mTFT->drawString(mItems[mLastSelected].c_str(),0,(mLastSelected - mMenuoffset) * mTFT->fontHeight() + 8);
    }
  }
  mLastSelected = mSelectedItem;
}
