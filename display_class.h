#ifndef _LCDDISPLAY_CLASS_H_
#define _LCDDISPLAY_CLASS_H_

#include <rgb_lcd.h>
#include "eh900_class.h"

/*! @class Eh_display
    @brief  ディスプレイを操作するためのクラス rgb_lcdクラスを継承
*/
class Eh_display : public rgb_lcd {

    public:
        // コンストラクタ  
        Eh_display(eh900* pModel);
        
        ~Eh_display(){
            Serial.println("~ Eh_display ----");
        }
    
        boolean init(uint16_t error);

        void showMeter(void);
        void showLevel(void);
        void showMode(void);
        void showTimer(void);
        void flashDisplay(void);

    private:
        eh900* LevelMeter = nullptr;

};

String right_align(String num_in_string, uint16_t digit);

#endif // _DISPLAY_CLASS_H_