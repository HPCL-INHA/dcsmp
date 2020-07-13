#ifndef DC_MQTT_H
#define DC_MQTT_H

////////////////////////////////////////
//
// Digital Companion MQTT C++ Header
//  - Jihong Min(alswg1994@gmail.com)
//
// (all values should be "string")
//
////////////////////////////////////////

#include <deque>
#include <string>

#define MQTT_KEEPALIVE_VAL 20
#define MQTT_QOS_LEVEL 1

typedef std::deque<std::string> MQTT_SUBTOPIC_LIST;
std::string MQTT_TOPIC_WITH_SUBTOPICS(std::string topic, MQTT_SUBTOPIC_LIST subtopics);
std::string MQTT_ALL_TOPICS_RELATED_WITH(std::string topic);

#define MQTT_DATETIME_STR_PRECISION 3
typedef struct MQTT_DATETIME
{
    unsigned long long time_int;
    double time_dec;
} MQTT_DATETIME;
void TRANSLATE_MQTT_DATETIME(MQTT_DATETIME datetime, int &year, int &mon, int &day, int &hour, int &min, double &sec);
MQTT_DATETIME TRANSLATE_MQTT_DATETIME_FROM_STRING(std::string datetime_str);
std::string TRANSLATE_MQTT_DATETIME_TO_STRING(MQTT_DATETIME datetime);
MQTT_DATETIME CREATE_MQTT_DATETIME(int year, int mon, int day, int hour, int min, double sec);
MQTT_DATETIME GET_CURRENT_MQTT_DATETIME();
MQTT_DATETIME ADD_MQTT_DATETIME_WITH_TIME(MQTT_DATETIME datetime, int days, int hours, int mins, double secs);
bool LATER_MQTT_DATETIME(MQTT_DATETIME first, MQTT_DATETIME second);
bool EQUAL_MQTT_DATETIME(MQTT_DATETIME first, MQTT_DATETIME second);

static const char MQTT_MSG_KEY_ACC_ACTION_LST[] = "accident_action_list";
static const char MQTT_MSG_KEY_ACTION_DATETIME[] = "timestamp";
static const char MQTT_MSG_KEY_ACTION_DEG[] = "inclination";
static const char MQTT_MSG_KEY_ACTION_DETECT_ANKLE_L[] = "l_ankle";
static const char MQTT_MSG_KEY_ACTION_DETECT_ANKLE_R[] = "r_ankle";
static const char MQTT_MSG_KEY_ACTION_DETECT_EAR_L[] = "l_hear";
static const char MQTT_MSG_KEY_ACTION_DETECT_EAR_R[] = "r_hear";
static const char MQTT_MSG_KEY_ACTION_DETECT_ELBOW_L[] = "l_elbow";
static const char MQTT_MSG_KEY_ACTION_DETECT_ELBOW_R[] = "r_elbow";
static const char MQTT_MSG_KEY_ACTION_DETECT_EYE_L[] = "l_eye";
static const char MQTT_MSG_KEY_ACTION_DETECT_EYE_R[] = "r_eye";
static const char MQTT_MSG_KEY_ACTION_DETECT_HIP_L[] = "l_hip";
static const char MQTT_MSG_KEY_ACTION_DETECT_HIP_R[] = "r_hip";
static const char MQTT_MSG_KEY_ACTION_DETECT_KNEE_L[] = "l_knee";
static const char MQTT_MSG_KEY_ACTION_DETECT_KNEE_R[] = "r_knee";
static const char MQTT_MSG_KEY_ACTION_DETECT_NOSE[] = "nose";
static const char MQTT_MSG_KEY_ACTION_DETECT_SHLDR_L[] = "l_shoulder";
static const char MQTT_MSG_KEY_ACTION_DETECT_SHLDR_R[] = "r_shoulder";
static const char MQTT_MSG_KEY_ACTION_DETECT_WRIST_L[] = "l_wrist";
static const char MQTT_MSG_KEY_ACTION_DETECT_WRIST_R[] = "r_wrist";
static const char MQTT_MSG_KEY_ACTION_DEVICE_ID[] = "deviceid";
static const char MQTT_MSG_KEY_ACTION_ODD[] = "odd_flag";
static const char MQTT_MSG_KEY_ACTION_SIG[] = "action";
static const char MQTT_MSG_KEY_ACTION_USER_ID[] = "userid";
static const char MQTT_MSG_KEY_ACTION_X_LEN[] = "x_length";
static const char MQTT_MSG_KEY_ACTION_Y_LEN[] = "y_length";
static const char MQTT_MSG_KEY_AX[] = "ax";
static const char MQTT_MSG_KEY_AY[] = "ay";
static const char MQTT_MSG_KEY_AZ[] = "az";
static const char MQTT_MSG_KEY_DATA[] = "data";
static const char MQTT_MSG_KEY_DATETIME[] = "datetime";
static const char MQTT_MSG_KEY_DEVICE_ID[] = "deviceId";
static const char MQTT_MSG_KEY_FALL_AMBIGUOUS[] = "FallingSuspicous";
static const char MQTT_MSG_KEY_FALL_TRUE[] = "Falling";
static const char MQTT_MSG_KEY_HEART_RATE[] = "ppg";
static const char MQTT_MSG_KEY_HEART_RATE_HIGH[] = "HighHeartRate";
static const char MQTT_MSG_KEY_HEART_RATE_LOW[] = "LowHeartRate";
static const char MQTT_MSG_KEY_HEART_RATE_STOP[] = "CardiacArrest";
static const char MQTT_MSG_KEY_INDEX[] = "index";
static const char MQTT_MSG_KEY_SENSOR_ID[] = "sensorId";
static const char MQTT_MSG_KEY_SENSOR_NO_CHANGE[] = "sensor_no_change";
static const char MQTT_MSG_KEY_STATE[] = "state";
static const char MQTT_MSG_KEY_STEP_CNT[] = "pedoCount";
static const char MQTT_MSG_KEY_USER_ID[] = "userId";

const char MQTT_ACTION_TOPIC[] = "buddybot/action1/";
const char MQTT_ACTION_TEST_TOPIC[] = "buddybot/action_test/";
enum MQTT_ACTION_SIG_ENUM
{
    ACTION_SITTING_FLOOR_TO_LYING = 0,
    ACTION_SITTING_FLOOR_TO_STANDING,
    ACTION_SITTING_FLOOR,
    ACTION_LYING_TO_SITTING_FLOOR,
    ACTION_LYING,
    ACTION_SITTING_CHAIR_TO_STANDING,
    ACTION_SITTING_CHAIR,
    ACTION_STANDING_TO_SITTING_FLOOR,
    ACTION_STANDING_TO_SITTING_CHAIR,
    ACTION_STANDING,
    ACTION_WALKING,
    ACTION_NO_SIG,
    ACTION_NO_SIG_READY,
    ACTION_NUM_SIGS
};
const struct
{
    const char *ACTION_SIG = MQTT_MSG_KEY_ACTION_SIG;
    const char *DATETIME = MQTT_MSG_KEY_ACTION_DATETIME;
    const char *DEG = MQTT_MSG_KEY_ACTION_DEG;
    const char *DETECT_ANKLE_L = MQTT_MSG_KEY_ACTION_DETECT_ANKLE_L;
    const char *DETECT_ANKLE_R = MQTT_MSG_KEY_ACTION_DETECT_ANKLE_R;
    const char *DETECT_EAR_L = MQTT_MSG_KEY_ACTION_DETECT_EAR_L;
    const char *DETECT_EAR_R = MQTT_MSG_KEY_ACTION_DETECT_EAR_R;
    const char *DETECT_ELBOW_L = MQTT_MSG_KEY_ACTION_DETECT_ELBOW_L;
    const char *DETECT_ELBOW_R = MQTT_MSG_KEY_ACTION_DETECT_ELBOW_R;
    const char *DETECT_EYE_L = MQTT_MSG_KEY_ACTION_DETECT_EYE_L;
    const char *DETECT_EYE_R = MQTT_MSG_KEY_ACTION_DETECT_EYE_R;
    const char *DETECT_HIP_L = MQTT_MSG_KEY_ACTION_DETECT_HIP_L;
    const char *DETECT_HIP_R = MQTT_MSG_KEY_ACTION_DETECT_HIP_R;
    const char *DETECT_KNEE_L = MQTT_MSG_KEY_ACTION_DETECT_KNEE_L;
    const char *DETECT_KNEE_R = MQTT_MSG_KEY_ACTION_DETECT_KNEE_R;
    const char *DETECT_NOSE = MQTT_MSG_KEY_ACTION_DETECT_NOSE;
    const char *DETECT_SHLDR_L = MQTT_MSG_KEY_ACTION_DETECT_SHLDR_L;
    const char *DETECT_SHLDR_R = MQTT_MSG_KEY_ACTION_DETECT_SHLDR_R;
    const char *DETECT_WRIST_L = MQTT_MSG_KEY_ACTION_DETECT_WRIST_L;
    const char *DETECT_WRIST_R = MQTT_MSG_KEY_ACTION_DETECT_WRIST_R;
    const char *DEVICE_ID = MQTT_MSG_KEY_ACTION_DEVICE_ID;
    const char *ODD = MQTT_MSG_KEY_ACTION_ODD;
    const char *USER_ID = MQTT_MSG_KEY_ACTION_USER_ID;
    const char *X_LEN = MQTT_MSG_KEY_ACTION_X_LEN;
    const char *Y_LEN = MQTT_MSG_KEY_ACTION_Y_LEN;
} MQTT_ACTION_MSG_KEYS;

const char MQTT_SENSOR_TOPIC[] = "buddybot/sensordata/";
const char MQTT_SENSOR_TEST_TOPIC[] = "buddybot/sensordata_test/";
const struct
{
    const char *DATETIME = MQTT_MSG_KEY_DATETIME;
    const char *DEVICE_ID = MQTT_MSG_KEY_DEVICE_ID;
    const char *HEART_RATE = MQTT_MSG_KEY_HEART_RATE;
    const char *STEP_CNT = MQTT_MSG_KEY_STEP_CNT;
    const char *SENSOR_ID = MQTT_MSG_KEY_SENSOR_ID;
    const char *STATE = MQTT_MSG_KEY_STATE;
    const char *USER_ID = MQTT_MSG_KEY_USER_ID;
    const char *DATA = MQTT_MSG_KEY_DATA;
    struct
    {
        const char *INDEX = MQTT_MSG_KEY_INDEX;
        const char *AX = MQTT_MSG_KEY_AX;
        const char *AY = MQTT_MSG_KEY_AY;
        const char *AZ = MQTT_MSG_KEY_AZ;
    } DATA_ELEM_KEYS;
} MQTT_SENSOR_MSG_KEYS;

const char MQTT_EAM_TOPIC[] = "buddybot/eam/";
const struct
{
    const char *ACC_ACTION_LIST = MQTT_MSG_KEY_ACC_ACTION_LST;
    const char *DATETIME = MQTT_MSG_KEY_DATETIME;
    const char *DEVICE_ID = MQTT_MSG_KEY_DEVICE_ID;
    const char *FALL_AMBIGUOUS = MQTT_MSG_KEY_FALL_AMBIGUOUS;
    const char *FALL_TRUE = MQTT_MSG_KEY_FALL_TRUE;
    const char *HEART_RATE_HIGH = MQTT_MSG_KEY_HEART_RATE_HIGH;
    const char *HEART_RATE_LOW = MQTT_MSG_KEY_HEART_RATE_LOW;
    const char *HEART_RATE_STOP = MQTT_MSG_KEY_HEART_RATE_STOP;
    const char *SENSOR_NO_CHANGE = MQTT_MSG_KEY_SENSOR_NO_CHANGE;
    const char *STATE = MQTT_MSG_KEY_STATE;
    const char *USER_ID = MQTT_MSG_KEY_USER_ID;
} MQTT_EAM_MSG_KEYS;

#endif
