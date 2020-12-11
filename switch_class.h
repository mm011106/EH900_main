#ifndef _SWITCH_CLASS_H_
#define _SWITCH_CLASS_H_

class Switch
{
public:
    Switch(uint16_t pio_port);

    // スイッチが接続されているポートIDを返す
    uint16_t getPortID(void){
       return port;
    };       

    // スイッチの押し続けられている時間を計測   ループの中で頻度高く呼び出す必要あり
    void updateStatus(void);

    // スイッチが押されている（押されていた）時間を返す
    uint32_t getDuration(void){
        // この変数は次にスイッチが押されるまで持続します。
        // clearDuration()  でクリアされます。
        return push_duration;
    };

    // スイッチが押されている時間の計測結果をクリアします
    void clearDuration(void){
        push_duration = 0;  
    };

    // スイッチの状態を返す (押されている = true, 離されている = false).
    boolean getStatus(void){
        return switch_status;
    };

    // スイッチは押されているのか？
    boolean isDepressed(void){
        return switch_status;
    };

    //  スイッチは離されているのか？
    boolean isReleased(void){
        return !switch_status;
    };
    
    // スイッチの状態が変化したか？
    //   状態を読み取るとフラグはクリアされます
    boolean hasChengedStatus(void);

    // スイッチは押されたか？
    //   状態を読み取るとフラグはクリアされます
    boolean hasDepressed(void);

    // スイッチは離されたか？
    //   状態を読み取るとフラグはクリアされます
    boolean hasReleased(void);
 
    //  スイッチ状態変化のフラグをクリア
    void clearChangeStatus(void);

    //  スイッチの状態をアップデート    スイッチ割り込みのISR内で実行すること
    void read_switch_status(void);

private:
    uint32_t push_duration;
    uint16_t port;
    uint32_t start_time;

    boolean switch_status;
    boolean change_of_state;
    boolean Depressed;
    boolean Released;


};

#endif // _SWITCH_CLASS_H_