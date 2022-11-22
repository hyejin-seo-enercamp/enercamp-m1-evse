#ifndef __CONTROL_PILOT_H__
#define __CONTROL_PILOT_H__

// CP Status
#define CP_DISCONNECTED     (0) ///< EVSE is not connected
#define CP_CONNECTING       (1) ///< EVSE is connecting
#define CP_CONNECTED        (2) ///< EVSE is connected
#define CP_CHARGING         (3) ///< Charging
#define CP_CHRAGING_DONE    (4) ///< Charging is done



void init_control_pilot();

/// @brief Get CP signal's duty ratio.
/// @return CP duty ratio (%)
int32_t get_cp_duty_ratio();

/// @brief Get CP signal's frequency.
/// @return CP frequency (Hz)
int32_t get_cp_frequency();

/// @brief Get CP status.
/// @return CP status
/// @retval CP_DISCONNECTED EVSE is not connected
/// @retval CP_CONNECTING EVSE is connecting
/// @retval CP_CONNECTED EVSE is connected
/// @retval CP_CHARGING EVSE is charging
/// @retval CP_CHARGING_DONE Chraging is done
int32_t get_cp_status();


#endif