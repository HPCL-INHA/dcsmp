#ifndef USERDAT_H
#define USERDAT_H

#include <deque>
#include <map>
#include <string>

#include "dc_mqtt/dc_mqtt.hpp"

#define ACCEL_EXCESS_BUFFER_SIZE_LIM 300
#define ACCEL_MAG_HIGH_THRESHOLD 2.5
#define ACCEL_MAG_LOW_THRESHOLD 1.0
#define ACCEL_NUM_EXPECT_ENTRIES 50
#define ACCEL_NO_MOVE_TRIG_DURATION_S 30 // 수정 필요

#define ACTION_BUFFER_LOOKAHEAD_TIME_BOUND_S 5
#define ACTION_BUFFER_LOOKBEHIND_TIME_BOUND_S 5
#define ACTION_BUFFER_LOOKAHEAD_MIN_CNT 0  // 수정 필요
#define ACTION_BUFFER_LOOKBEHIND_MIN_CNT 0 // 수정 필요
#define ACTION_BUFFER_SIZE_LIM 300

#define FALL_NEXT_INVALID_ACTION_THRESHOLD 1
#define FALL_PREV_INVALID_ACTION_THRESHOLD 1
#define FALL_PRED_AMBIGUOUS_RATIO_THRESHOLD 0.7
#define FALL_PRED_TRUE_RATIO_THRESHOLD 0.8

#define HEART_RATE_UPPER_THRESHOLD 160 // 수정 필요
#define HEART_RATE_LOWER_THRESHOLD 60  // 수정 필요
#define HEART_RATE_MIN_THRESHOLD 10    // 수정 필요

enum FALL_PROB_ENUM
{
    FALL_NORMAL = 0,
    FALL_UNDETERMINED,
    FALL_AMBIGUOUS,
    FALL_TRUE
};

enum HEART_PROB_ENUM
{
    HEART_NORMAL = 0,
    HEART_RATE_HIGH,
    HEART_RATE_LOW,
    HEART_RATE_STOP
};

typedef struct ACCEL
{
    double mag;
    MQTT_DATETIME datetime;
} ACCEL;

typedef struct ACTION
{
    MQTT_ACTION_SIG_ENUM sig;
    MQTT_DATETIME datetime;
} ACTION;

typedef struct HEART_COND
{
    MQTT_DATETIME datetime;
    int heart_rate;
} HEART_COND;

class User
{
private:
    std::string device_id = "";

    std::deque<ACTION> action_buffer;
    size_t cnt_next_action = 0;
    size_t cnt_prev_action = 0;
    bool popped_past_action = false;

    std::deque<ACCEL> excess_accel_buffer;
    size_t cnt_low_accel = 0;

    std::deque<HEART_COND> abnorm_heart_conds;

    void setDeviceID(std::string device_id);

    HEART_PROB_ENUM checkAbnormalHeartCondition(HEART_COND heart_cond) const;

public:
    User();
    User(std::string device_id);

    std::string getDeviceID() const;

    FALL_PROB_ENUM popFallCondition(MQTT_DATETIME &incident_datetime, std::deque<ACTION> &action_buffer_bak);
    void storeAccelerationIfExcessive(ACCEL accel);
    void storeActionIntoBuffer(ACTION action);
    bool testNoRecentMovement();

    HEART_PROB_ENUM popAbnormalHeartCondition(MQTT_DATETIME &incident_datetime);
    void storeAbnormalHeartCondition(HEART_COND heart_cond);
};
typedef std::map<std::string, User> USER_DATA;

#endif
