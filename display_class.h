#ifndef _LCDDISPLAY_CLASS_H_
#define _LCDDISPLAY_CLASS_H_

#include <rgb_lcd.h>
#include "eh900_class.h"

class Eh_display : public rgb_lcd {

    public:
        // コンストラクタ  
        Eh_display(eh900* pModel);
        
        ~Eh_display(){
            Serial.println("~ Eh_display ----");
        }
        
        //  LCDのイニシャライズ  液面計オブジェクトを渡す
        //      error が0以外ならエラーを表示する
        void init(uint16_t error);

        //  液面計の表示に設定する
        void showMeter(void);

        ///  液面レベルの表示  数値＋バーグラフ
        /// 電流源のエラー表示
        void showLevel(void);

        //  モードの表示 100ms程度で周期的に呼ぶことでスムースな表示になる
        void showMode(void);

        void showTimer(void);

        //  画面全体をフラッシュさせる  300ms必要
        void flashDisplay(void);

    private:
        eh900* LevelMeter = nullptr;

};

//  指定桁数の数字（ストリング）を右寄せで表示
String right_align(String num_in_string, uint16_t digit);

#endif // _DISPLAY_CLASS_H_