#include <Arduino.h>
#include "switch_class.h"

Switch::Switch(uint16_t PIO_for_switch){
    
    port = PIO_for_switch;
    pinMode(port, INPUT_PULLUP);

}    

void Switch::updateStatus(void){
    uint32_t now = 0;

    // measure time duration of the switch depressed.
    if (switch_status){
        now = millis();
        if ( start_time > now ){
            //  in case of the millis() was wrapped around.
            push_duration = 4294967295 - start_time + now;
        } else {
            push_duration = now - start_time;
        }
    }
}


// activated when switch status changed. valid only once.
boolean Switch::hasChengedStatus(void){
    const boolean flag = change_of_state;

    change_of_state = false;
    return flag;
}

boolean Switch::hasDepressed(void){
    const boolean flag = Depressed;

    Depressed = false;
    return flag;
}

boolean Switch::hasReleased(void){
    const boolean flag = Released;

    Released = false;
    return flag;
}

void Switch::clearChangeStatus(void){
    Released = false;
    Depressed = false;
    change_of_state = false;
}

//  update status of switch, attached to ISR of switch
void Switch::read_switch_status(void){
    

    if ((! switch_status) && digitalRead(port)==HIGH) {
        switch_status = true;
        change_of_state = true;
        Depressed = true;
        Released = false;
        start_time = millis();
    }

      if ((switch_status) && digitalRead(port)==LOW) {
        switch_status = false;
        change_of_state = true;
        Released = true;
        Depressed = false;
    }


}

