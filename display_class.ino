#include "display_class.h"

namespace{
    constexpr uint16_t POSITION_BAR_GRAPH       = 10;
    constexpr uint16_t POSITION_SENSOR_LENGTH   = 1;
    constexpr uint16_t POSITION_TIMER_SET       = 5;
    constexpr uint16_t POSITION_TIMER_COUNT     = 2;
    constexpr uint16_t POSITION_LEVEL           = 10;
    constexpr uint16_t POSITION_MODE            = 0;

    byte bar_graph[5][8] = {
        {
            0b10000,
            0b10000,
            0b10000,
            0b10000,
            0b10000,
            0b10000,
            0b10000,
            0b10000 
        },
        {
            0b11000,
            0b11000,
            0b11000,
            0b11000,
            0b11000,
            0b11000,
            0b11000,
            0b11000
        },
        {
            0b11100,
            0b11100,
            0b11100,
            0b11100,
            0b11100,
            0b11100,
            0b11100,
            0b11100
        },   
        {
            0b11110,
            0b11110,
            0b11110,
            0b11110,
            0b11110,
            0b11110,
            0b11110,
            0b11110
        },   
        {
            0b11111,
            0b11111,
            0b11111,
            0b11111,
            0b11111,
            0b11111,
            0b11111,
            0b11111
        },
    };

}

// LcdDisplay::LcdDisplay(const uint16_t* level){
Eh_display::Eh_display(){

}

// initialize LCD display as 2 line, 16 column.
void Eh_display::init(eh900* pModel, uint16_t error){
    
    pMeter = pModel;

    rgb_lcd::begin(16,2);

    for (int i=0; i<5; i++){
        rgb_lcd::createChar(i, bar_graph[i]);
    }

    rgb_lcd::clear();
    rgb_lcd::setCursor(2, 0);

    rgb_lcd::print("-- EH-900 --");
    if(error != 0){
        rgb_lcd::setCursor(3, 1);
        rgb_lcd::print("INIT ERR:");
        rgb_lcd::print(right_align(String( error ), 2));
    }
}

void Eh_display::showMeter(void){
        rgb_lcd::setCursor(0, 0);
        rgb_lcd::print(" :  /   E:    :F");

        rgb_lcd::setCursor(POSITION_TIMER_SET,0);
        rgb_lcd::print(right_align(String( pMeter->getTimerPeriod() / 60 ), 2));

        rgb_lcd::setCursor(POSITION_SENSOR_LENGTH,1);
        rgb_lcd::print(right_align(String( pMeter->getSensorLength() ), 2));
        rgb_lcd::print("inch   ");
}

void Eh_display::showLevel(void){
    
    uint16_t value = pMeter->getLiquidLevel();

    rgb_lcd::setCursor(POSITION_LEVEL, 1);
    rgb_lcd::print(right_align(String((float)value/10.0,1),5));
    rgb_lcd::print("%");


    rgb_lcd::setCursor(POSITION_BAR_GRAPH, 0);
    // clear bar display area
    rgb_lcd::print("    ");

    uint16_t fine = (value % 250)/50;
    uint16_t coarse = value / 250;
    uint16_t x_pos = 0;

    // block of 25%
    for (x_pos=0; x_pos<coarse; x_pos++){
        rgb_lcd::setCursor(POSITION_BAR_GRAPH+x_pos,0);
        rgb_lcd::write((unsigned char)4);
    };
    // block of 5%
    if (fine>0){
        rgb_lcd::setCursor(POSITION_BAR_GRAPH+x_pos,0);
        rgb_lcd::write((unsigned char)(fine-1));
    };

    //  センサエラー表示
    if(pMeter->isSensorError()){
        rgb_lcd::setCursor(POSITION_SENSOR_LENGTH,1);
        rgb_lcd::print("-ERROR-");
    } else {
        rgb_lcd::setCursor(POSITION_SENSOR_LENGTH,1);
        rgb_lcd::print(right_align(String( pMeter->getSensorLength() ), 2));
        rgb_lcd::print("inch   ");
    }
    
}

void Eh_display::showMode(void){

    rgb_lcd::setCursor(POSITION_MODE,0);

    // 連続モードの時にフラッシュする   1sec周期でブリンク
    if ( ( pMeter->getMode() == Continuous ) && ( millis()%1000 < 500 )){
        rgb_lcd::print(" ");
    } else {
        rgb_lcd::print(ModeNames[pMeter->getMode()]);
    }      

    // タイマーモードの時のtick-tock 
    rgb_lcd::setCursor(POSITION_MODE+1,0);
    // 2sec周期でブリンク
    if ( ( pMeter->getMode() == Timer ) && ( millis()%2000 < 1000 )){
        rgb_lcd::write(" ");
    } else {
        rgb_lcd::write(":");
    }

}

void Eh_display::showTimer(void){
    rgb_lcd::setCursor(POSITION_TIMER_COUNT,0);
    rgb_lcd::print(right_align(String(pMeter->getTimerElasped() / 60),2));

}

void Eh_display::flashDisplay(void){
    constexpr uint16_t interval = 200;
    rgb_lcd::noDisplay();
    delay(interval + 100);
    rgb_lcd::display();
    delay(interval);
}

String right_align(String num_in_string, uint16_t digit){

    String right_aligned="";
    String pad="";

    if (digit>1){
        //  string pad(digit-1, ' '); と同じ  arduino のstringはこれができない
        for (uint16_t i=0; i< digit-1; i++){
            pad += " ";
        }

        pad += num_in_string;

        for (uint16_t i=digit ; i> 0 ; i--){
            right_aligned += pad[pad.length()-i];
        }
    }

    return right_aligned;
}



