/*
  Buzzer driver
*/
/*
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __BUZZER_H__
#define __BUZZER_H__

#if CONFIG_HAL_BOARD == HAL_BOARD_APM1
# define BUZZER_PIN     63      // pin 63 on APM1
#elif CONFIG_HAL_BOARD == HAL_BOARD_APM2
 # define BUZZER_PIN    59      // pin 59 on APM2
#elif CONFIG_HAL_BOARD == HAL_BOARD_MPNG
# define BUZZER_PIN    57      // pin 59 on APM2
#else
 # define BUZZER_PIN    0       // pin undefined on other boards
#endif

class Buzzer
{
public:
    /// Constructor
    Buzzer() : _counter(0), _pattern(NONE), _pattern_counter(0) {}

    /// init - initialise the buzzer
    void init(void);

    /// update - updates buzzer according to timed_updated.  Should be called at 50Hz
    void update();

    /// on - turns the buzzer on or off
    void on(bool on_off);

    enum BuzzerPattern {
        NONE = 0,
        SINGLE_BUZZ = 1,
        DOUBLE_BUZZ = 2,
        GPS_GLITCH = 3
    };

    /// play_pattern - plays the defined buzzer pattern
    void play_pattern(BuzzerPattern pattern_id);

private:

    /// buzzer_flag_type - bitmask of current state and ap_notify states we track
    struct buzzer_flag_type {
        uint8_t on                  : 1;    // 1 if the buzzer is currently on
        uint8_t gps_glitching       : 1;    // 1 if gps position is not good
        uint8_t arming              : 1;    // 1 if we are beginning the arming process
        uint8_t armed               : 1;    // 0 = disarmed, 1 = armed
        uint8_t failsafe_battery    : 1;    // 1 if battery failsafe has triggered
        uint8_t failsafe_gps        : 1;    // 1 if gps failsafe
    } _flags;

    uint8_t         _counter;           // reduces 50hz update down to 10hz for internal processing
    BuzzerPattern   _pattern;           // current pattern
    uint8_t         _pattern_counter;   // used to time on/off of current patter
};

#endif // __BUZZER_H__
