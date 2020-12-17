/**
 * @file eh900_config.h
 * @brief eh900 のパラメタを手動で変更するためのコンフィギュレーションメニューを提供
 * @author miyamoto
 * @date 2020/12/17
 */

#ifndef _EH900_CONFIG_H_
#define _EH900_CONFIG_H_

#include "switch_class.h"
#include "display_class.h"
#include "eh900_class.h"

//! スイッチの状態を知るためのクラス
extern Switch meas_sw;

//! 液晶表示器を操作するためのクラス
extern Eh_display lcd_display;

//! eh900のパラメタを設定／保存するためのクラス
extern eh900 level_meter;

void menu_main(void);

void menu_timer_period(void);

void menu_sensor_length(void);


#endif // _EH900_CONFIG_H_