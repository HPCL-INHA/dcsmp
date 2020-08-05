///////////////////////////////////////////////////////
//
// DCSMP(Digital Companion Sensor Message Processor)
//  - Jihong Min(alswg1994@gmail.com)
//
// To-do List:
//  1. Multi-device 지원 + Multi-sensor 지원(?)
//  2. 심박수 Check 하드웨어 버그 수정되면 Value 다시 조절
//  3. Action 없는 상황에서 Excessive Sensor 데이터 드랍하기
//  4. ACCEL_NUM_MIN_SEQ_CNT 사용 구현하기
//  5. 사고 Detection 과정 최적화
//  6. Makefile 수정
//  7. 전체적 리팩터링(코드, 입출력, MQTT 메세지 포맷 포함)
//
///////////////////////////////////////////////////////

using namespace std;

/***************************************************************************/
#define VERSION "0.9.1"

#ifdef DEBUG
#define DEBUG_ACCEL_MAG_STR_PRECISION 6
const char DEBUG_ACTION_MSG_DUMP_FILE_NAME[] = "debug_action_message.dmp";
const char DEBUG_EAM_MSG_DUMP_FILE_NAME[] = "debug_eam_message.dmp";
const char DEBUG_SENSOR_MSG_DUMP_FILE_NAME[] = "debug_sensor_message.dmp";

#include <fstream>
ofstream debug_ofs_action;
ofstream debug_ofs_eam;
ofstream debug_ofs_sensor;
#endif

#ifdef TEST
const char TEST_DEVICE_ID[] = "deviceId";
const char TEST_SENSOR_ID[] = "sensorId";
const char TEST_USER_ID[] = "userId";

#define TEST_ACTION_POLLING_RATE_HZ 10
#define TEST_HEART_RATE_LIM 220 // 수정 필요
#define TEST_MSG_DELAY_TIME_S 1
#define TEST_NUM_SENSOR_DATA_ENTRIES ACCEL_NUM_EXPECT_ENTRIES
int test_step_cnt = 0;
#define TEST_STEP_GROWTH_LIM 10 // 수정 필요(?)
#define TEST_XYZ_LIM 1.65
#endif

const char CLIENT__ID[24] = "DCSMP";
#define STARTUP_DELAY_TIME_S 1
#define LOOP_DELAY_TIME_MICRO_S 100000
#define TERM_DELAY_TIME_S 1
/***************************************************************************/

#include <cmath>
#include <csignal>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>

#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>

#include <jsoncpp/json/json.h>
#include <mosquitto.h>

#include "dc_mqtt/dc_mqtt.hpp"      // for communicating with Digital Companion MQTT server
#include "userdata.hpp"             // for user data processing
#include "xyz.hpp"                  // for 3-dimensional calculation

bool mosq_running = true;
void mosq_terminate(int sig);
void mosq_connect_callback(struct mosquitto *mosq, void *obj, int result);
void mosq_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg);

USER_DATA global_user_data;
int main(int argc, char *argv[])
{
    if(argc != 5){
        cout << "Usage: " << argv[0] << " DOMAIN PORT ID PSWD" << endl;
        return EXIT_SUCCESS;
    }
    const string SERVER_DOMAIN = argv[1];
    const string SERVER_PORT = argv[2];
    const string SERVER_ID = argv[3];
    const string SERVER_PSWD = argv[4];
    cout << "SERVER_DOMAIN: " << argv[1] << endl;
    cout << "SERVER_PORT: " << argv[2] << endl;
    cout << "SERVER_ID: " << argv[3] << endl;
    cout << "SERVER_PSWD: " << argv[4] << endl;

    /*    C++ Standard I/O Optimization    */
    /* DO NOT USE C STANDARD I/O FUNCTION! */
    ios::sync_with_stdio(false);
    cin.tie(NULL);
    cout.tie(NULL);
    /* END */

#ifdef DEBUG
    debug_ofs_action.open(DEBUG_ACTION_MSG_DUMP_FILE_NAME, ios::trunc);
    debug_ofs_eam.open(DEBUG_EAM_MSG_DUMP_FILE_NAME, ios::trunc);
    debug_ofs_sensor.open(DEBUG_SENSOR_MSG_DUMP_FILE_NAME, ios::trunc);
#endif

#ifdef TEST
    srand(time(NULL));
#endif

    /* Welcome Message */
    cout << endl
         << "\033[1m"
         << "\tDigital Companion Sensor Message Processor " << VERSION;
#ifdef TEST
    cout << " - SELF TEST MODE";
#endif
    cout << "\033[0m" << endl
         << endl;
#ifdef TEST
    cout << "MQTT_ACTION_TEST_TOPIC: " << MQTT_ACTION_TEST_TOPIC << endl;
    cout << "MQTT_EAM_TEST_TOPIC: " << MQTT_EAM_TEST_TOPIC << endl;
    cout << "MQTT_SENSOR_TEST_TOPIC: " << MQTT_SENSOR_TEST_TOPIC << endl;
#else
    cout << "MQTT_ACTION_TOPIC: " << MQTT_ACTION_TOPIC << endl;
    cout << "MQTT_EAM_TOPIC: " << MQTT_EAM_TOPIC << endl;
    cout << "MQTT_SENSOR_TOPIC: " << MQTT_SENSOR_TOPIC << endl;
#endif
    cout <<endl;
    /* END */

    /* DNS Query */
    struct hostent *host_entry;
    host_entry = gethostbyname(SERVER_DOMAIN.c_str());
    if (!host_entry)
    {
        cerr << "[FAILED] gethostbyname(): unable to obtain address of the server domain" << endl;
        return EXIT_FAILURE;
    }
    string mqtt_server_ip = inet_ntoa(*(struct in_addr *)host_entry->h_addr_list[0]);
    cout << "\033[1;32m"
         << "DNS query returned successfully!" << endl
         << "SERVER IP: " << mqtt_server_ip.c_str() << "\033[0m" << endl
         << endl;
    /* END */

    /* Set Signal Handler for Termination */
    signal(SIGTSTP, mosq_terminate);
    signal(SIGINT, mosq_terminate);
    signal(SIGQUIT, mosq_terminate);
    /* END */

    /* Initialize Mosquitto */
    mosquitto_lib_init();
    /* END */

    /* Create MQTT Broker */
    struct mosquitto *mosq;
    mosq = mosquitto_new(CLIENT__ID, true, NULL);
    if (!mosq)
    {
        cerr << "[FAILED] mosquitto_new(): unable to initialize new mosquitto structure" << endl;
        return EXIT_FAILURE;
    }
    /* END */

    /* Set callbacks for Mosquitto */
    mosquitto_connect_callback_set(mosq, mosq_connect_callback);
    mosquitto_message_callback_set(mosq, mosq_message_callback);
    /* END */

    /* Set authorization _ID and password for Mosquitto */
    int ret = mosquitto_username_pw_set(mosq, SERVER_ID.c_str(), SERVER_PSWD.c_str());
    if (ret)
    {
        cerr << "[FAILED] mosquitto_username_pw_set(): returned " << ret << endl;
        return EXIT_FAILURE;
    }
    /* END */

    /* Connect to MQTT Server */
    ret = mosquitto_connect(mosq, mqtt_server_ip.c_str(), atoi(SERVER_PORT.c_str()), MQTT_KEEPALIVE_VAL);
    if (ret)
    {
        cerr << "[FAILED] mosquitto_connect(): returned " << ret << endl;
        return EXIT_FAILURE;
    }
    /* END */

    /* Subscribe MQTT Sensor Topic */
#ifdef TEST
    ret = mosquitto_subscribe(mosq, NULL, MQTT_ALL_TOPICS_RELATED_WITH(MQTT_SENSOR_TEST_TOPIC).c_str(), 0);
#else
    ret = mosquitto_subscribe(mosq, NULL, MQTT_ALL_TOPICS_RELATED_WITH(MQTT_SENSOR_TOPIC).c_str(), 0);
#endif
    if (ret)
    {
        cerr << "[FAILED] mosquitto_subscribe(): returned " << ret << endl;
        exit(EXIT_FAILURE);
    }
    /* END */

    /* Subscribe MQTT Action Topic */
#ifdef TEST
    ret = mosquitto_subscribe(mosq, NULL, MQTT_ALL_TOPICS_RELATED_WITH(MQTT_ACTION_TEST_TOPIC).c_str(), 0);
#else
    ret = mosquitto_subscribe(mosq, NULL, MQTT_ALL_TOPICS_RELATED_WITH(MQTT_ACTION_TOPIC).c_str(), 0);
#endif
    if (ret)
    {
        cerr << "[FAILED] mosquitto_subscribe(): returned " << ret << endl;
        exit(EXIT_FAILURE);
    }
    /* END */

    /* Sleep for Startup Delay */
    sleep(STARTUP_DELAY_TIME_S);
    /* END */

    while (mosq_running)
    {
#ifdef TEST
        /* Template Sensor Message Generation */
        Json::Value sensor_msg_root;
        sensor_msg_root[MQTT_SENSOR_MSG_KEYS.DEVICE_ID] = TEST_DEVICE_ID;
        sensor_msg_root[MQTT_SENSOR_MSG_KEYS.SENSOR_ID] = TEST_SENSOR_ID;
        sensor_msg_root[MQTT_SENSOR_MSG_KEYS.USER_ID] = TEST_USER_ID;
        /* END */

        /* Sensor Datetime Generation */
        MQTT_DATETIME cur_datetime = GET_CURRENT_MQTT_DATETIME();
        sensor_msg_root[MQTT_SENSOR_MSG_KEYS.DATETIME] = TRANSLATE_MQTT_DATETIME_TO_STRING(cur_datetime);
        /* END */

        /* Data Vector Generation */
        Json::Value sensor_msg_data(Json::arrayValue);
        size_t test_max_index = rand() % (TEST_NUM_SENSOR_DATA_ENTRIES);
        for (size_t index = 0; index <= test_max_index; index++)
        {
            Json::Value entry;
            entry[MQTT_SENSOR_MSG_KEYS.DATA_ELEM_KEYS.INDEX] = to_string(index);
            entry[MQTT_SENSOR_MSG_KEYS.DATA_ELEM_KEYS.AX] = to_string((double)rand() * TEST_XYZ_LIM / RAND_MAX);
            entry[MQTT_SENSOR_MSG_KEYS.DATA_ELEM_KEYS.AY] = to_string((double)rand() * TEST_XYZ_LIM / RAND_MAX);
            entry[MQTT_SENSOR_MSG_KEYS.DATA_ELEM_KEYS.AZ] = to_string((double)rand() * TEST_XYZ_LIM / RAND_MAX);
            sensor_msg_data.append(entry);
        }
        sensor_msg_root[MQTT_SENSOR_MSG_KEYS.DATA] = sensor_msg_data;
        sensor_msg_root[MQTT_SENSOR_MSG_KEYS.STATE] = (test_max_index + 1 == TEST_NUM_SENSOR_DATA_ENTRIES) ? "1" : "0";
        /* END */

        /* Heart Rate & Step Counter Generation */
        sensor_msg_root[MQTT_SENSOR_MSG_KEYS.HEART_RATE] = to_string(rand() % (TEST_HEART_RATE_LIM + 1));
        test_step_cnt += rand() % (TEST_STEP_GROWTH_LIM + 1);
        sensor_msg_root[MQTT_SENSOR_MSG_KEYS.STEP_CNT] = to_string(test_step_cnt);
        /* END */

        /* Publish Sensor Test Message to MQTT Sensor Test Topic */
        stringstream sensor_msg_ss;
        sensor_msg_ss << sensor_msg_root;
        MQTT_SUBTOPIC_LIST sensor_subtopics;
        sensor_subtopics.push_back(TEST_USER_ID);
        sensor_subtopics.push_back(TEST_DEVICE_ID);
        mosquitto_publish(mosq, NULL, MQTT_TOPIC_WITH_SUBTOPICS(MQTT_SENSOR_TEST_TOPIC, sensor_subtopics).c_str(), sensor_msg_ss.str().size(), sensor_msg_ss.str().c_str(), MQTT_QOS_LEVEL, false);
        /* END */

        for (int i = 0; i < TEST_ACTION_POLLING_RATE_HZ; i++)
        {
            /* Template Action Message Generation */
            Json::Value action_msg_root;
            action_msg_root[MQTT_ACTION_MSG_KEYS.DEVICE_ID] = TEST_DEVICE_ID;
            action_msg_root[MQTT_ACTION_MSG_KEYS.USER_ID] = TEST_USER_ID;
            /* END */

            /* Action Datetime Generation */
            double time_offset = (double)i / TEST_ACTION_POLLING_RATE_HZ;
            MQTT_DATETIME action_datetime = ADD_MQTT_DATETIME_WITH_TIME(cur_datetime, 0, 0, 0, time_offset);
            action_msg_root[MQTT_ACTION_MSG_KEYS.DATETIME] = TRANSLATE_MQTT_DATETIME_TO_STRING(action_datetime);
            /* END */

            /* Action Signal Generation */
            action_msg_root[MQTT_ACTION_MSG_KEYS.ACTION_SIG] = to_string(rand() % ACTION_NUM_SIGS);
            /* END */

            /* Publish Action Test Message to MQTT Action Test Topic */
            stringstream action_msg_ss;
            action_msg_ss << action_msg_root;
            MQTT_SUBTOPIC_LIST action_subtopics;
            action_subtopics.push_back(TEST_USER_ID);
            action_subtopics.push_back(TEST_DEVICE_ID);
            mosquitto_publish(mosq, NULL, MQTT_TOPIC_WITH_SUBTOPICS(MQTT_ACTION_TEST_TOPIC, action_subtopics).c_str(), action_msg_ss.str().size(), action_msg_ss.str().c_str(), MQTT_QOS_LEVEL, false);
            /* END */
        }

        /* Sleep for Message Delay */
        sleep(TEST_MSG_DELAY_TIME_S);
        /* END */
#endif

        /* Loop MQTT Communication */
        mosquitto_loop(mosq, -1, 1);
        /* END */

        for (USER_DATA::iterator glob_user_data_iter = global_user_data.begin(); glob_user_data_iter != global_user_data.end(); glob_user_data_iter++)
        {
            /* Get User ID & Device ID(currently only one device per user) */
            string user_id = glob_user_data_iter->first;
            string device_id = glob_user_data_iter->second.getDeviceID();
            /* END */

            bool sensor_no_change = glob_user_data_iter->second.testNoRecentMovement();

            MQTT_DATETIME falling_incident_datetime, heart_incident_datetime;
            deque<ACTION> action_buffer_bak;
            HEART_PROB_ENUM heart_status = glob_user_data_iter->second.popAbnormalHeartCondition(heart_incident_datetime);
            FALL_PROB_ENUM falling_status = glob_user_data_iter->second.popFallCondition(falling_incident_datetime, action_buffer_bak);
            bool fall_true = false, fall_ambiguous = false, heart_rate_high = false, heart_rate_low = false, heart_rate_stop = false,
                 use_falling_time = true, use_heart_time = true;

            switch (falling_status)
            {
            case FALL_AMBIGUOUS:
                fall_ambiguous = true;
                break;
            case FALL_TRUE:
                fall_true = true;
                break;
            case FALL_NORMAL:
            case FALL_UNDETERMINED:
                use_falling_time = false;
            }

            switch (heart_status)
            {
            case HEART_RATE_HIGH:
                heart_rate_high = true;
                break;
            case HEART_RATE_LOW:
                heart_rate_low = true;
                break;
            case HEART_RATE_STOP:
                heart_rate_stop = true;
                break;
            case HEART_NORMAL:
                use_heart_time = false;
            }

            /* Send EAM Message */
            if (sensor_no_change || use_falling_time || use_heart_time)
            {
#ifdef DEBUG
                cout << "\033[1;31m"
                     << "[DEBUG] Sending EAM Message..." << endl
                     << "[DEBUG] User ID: " << user_id << endl
                     << "[DEBUG] Device ID: " << device_id << endl;
                if (fall_ambiguous)
                {
                    cout << "[DEBUG] fall_ambiguous: " << fall_ambiguous << endl;
                }
                if (fall_true)
                {
                    cout << "[DEBUG] fall_true: " << fall_true << endl;
                }
                if (heart_rate_high)
                {
                    cout << "[DEBUG] heart_rate_high: " << heart_rate_high << endl;
                }
                if (heart_rate_low)
                {
                    cout << "[DEBUG] heart_rate_low: " << heart_rate_low << endl;
                }
                if (heart_rate_stop)
                {
                    cout << "[DEBUG] heart_rate_stop: " << heart_rate_stop << endl;
                }
                if (sensor_no_change)
                {
                    cout << "[DEBUG] sensor_no_change: " << sensor_no_change << endl;
                }
                cout << "\033[0m";
#endif

                Json::Value eam_msg_root;
                eam_msg_root[MQTT_EAM_MSG_KEYS.DEVICE_ID] = device_id;
                eam_msg_root[MQTT_EAM_MSG_KEYS.USER_ID] = user_id;
                if (use_heart_time)
                {
                    eam_msg_root[MQTT_EAM_MSG_KEYS.DATETIME] = TRANSLATE_MQTT_DATETIME_TO_STRING(heart_incident_datetime);
                }
                else if (use_falling_time)
                {
                    eam_msg_root[MQTT_EAM_MSG_KEYS.DATETIME] = TRANSLATE_MQTT_DATETIME_TO_STRING(falling_incident_datetime);
                }
                else
                {
                    eam_msg_root[MQTT_EAM_MSG_KEYS.DATETIME] = TRANSLATE_MQTT_DATETIME_TO_STRING(GET_CURRENT_MQTT_DATETIME());
                }
                eam_msg_root[MQTT_EAM_MSG_KEYS.FALL_AMBIGUOUS] = fall_ambiguous;
                eam_msg_root[MQTT_EAM_MSG_KEYS.FALL_TRUE] = fall_true;
                eam_msg_root[MQTT_EAM_MSG_KEYS.HEART_RATE_HIGH] = heart_rate_high;
                eam_msg_root[MQTT_EAM_MSG_KEYS.HEART_RATE_LOW] = heart_rate_low;
                eam_msg_root[MQTT_EAM_MSG_KEYS.HEART_RATE_STOP] = heart_rate_stop;
                eam_msg_root[MQTT_EAM_MSG_KEYS.SENSOR_NO_CHANGE] = sensor_no_change;

                Json::Value action_buffer_bak_array(Json::arrayValue);
                for(auto iter = action_buffer_bak.begin(); iter != action_buffer_bak.end(); iter++){
                    action_buffer_bak_array.append(iter->sig);
                }
                eam_msg_root[MQTT_EAM_MSG_KEYS.ACC_ACTION_LIST] = action_buffer_bak_array;

                stringstream eam_msg_ss;
                eam_msg_ss << eam_msg_root;
#ifdef DEBUG
                debug_ofs_eam << eam_msg_root;
#endif
                MQTT_SUBTOPIC_LIST subtopics;
                subtopics.push_back(user_id);
                subtopics.push_back(device_id);
                mosquitto_publish(mosq, NULL, MQTT_TOPIC_WITH_SUBTOPICS(MQTT_EAM_TOPIC, subtopics).c_str(), eam_msg_ss.str().size(), eam_msg_ss.str().c_str(), MQTT_QOS_LEVEL, false);
            }
            /* END */
        }

        /* Sleep for Delay */
        usleep(LOOP_DELAY_TIME_MICRO_S);
        /* END */
    }

    /* Termination */
    sleep(TERM_DELAY_TIME_S);
    mosquitto_disconnect(mosq);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    return EXIT_SUCCESS;
    /* END */
}

void mosq_terminate(int sig)
{
    cout << endl
         << "\033[1;33m"
         << "Termination signal has occured!"
         << "\033[0m" << endl
         << endl;
    mosq_running = false;
}

void mosq_connect_callback(struct mosquitto *mosq, void *obj, int result)
{
    cout << "\033[1;32m"
         << "MQTT broker has been established!"
         << "\033[0m" << endl
         << endl;
}

void mosq_message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *msg)
{
    /* Get Message Information */
    string msg_topic = msg->topic;
    stringstream msg_root_ss;
    msg_root_ss << (char *)msg->payload;
    Json::Value msg_root;
    msg_root_ss >> msg_root;
    /* END */

    /* MQTT Action Message Reception */
#ifdef TEST
    if (msg_topic.find(MQTT_ACTION_TEST_TOPIC) != msg_topic.npos)
#else
    if (msg_topic.find(MQTT_ACTION_TOPIC) != msg_topic.npos)
#endif
    {
#ifdef DEBUG
        /* Dump Action Message */
        debug_ofs_action << msg_root << endl;
        /* END */
#endif

        /* Get Action User ID & Device ID(currently only 1 device per user is allowed) */
        string user_id = msg_root[MQTT_ACTION_MSG_KEYS.USER_ID].asString();
        string device_id = msg_root[MQTT_ACTION_MSG_KEYS.DEVICE_ID].asString();
        if (global_user_data.find(user_id) == global_user_data.end())
        {
            global_user_data[user_id] = User(device_id);
        }
        /* END */

        /* Store Action Data into Queue */
        ACTION action;
        action.sig = (MQTT_ACTION_SIG_ENUM)atoi(msg_root[MQTT_ACTION_MSG_KEYS.ACTION_SIG].asString().c_str());
        action.datetime = TRANSLATE_MQTT_DATETIME_FROM_STRING(msg_root[MQTT_ACTION_MSG_KEYS.DATETIME].asString());
        global_user_data[user_id].storeActionIntoBuffer(action);
        /* END */

#ifdef DEBUG
        /* Print Action Debug Message to Console */
        cout << "[DEBUG] Action Message Reception -> User ID: " << user_id << " / Device ID: " << device_id;
        cout << " / Signal: ";
        switch (action.sig)
        {
        case ACTION_WALKING:
        case ACTION_STANDING:
        case ACTION_STANDING_TO_SITTING_CHAIR:
        case ACTION_SITTING_CHAIR:
        case ACTION_SITTING_CHAIR_TO_STANDING:
            cout << "\033[1;34m";
            break;
        case ACTION_SITTING_FLOOR_TO_LYING:
        case ACTION_SITTING_FLOOR_TO_STANDING:
        case ACTION_LYING_TO_SITTING_FLOOR:
        case ACTION_LYING:
        case ACTION_STANDING_TO_SITTING_FLOOR:
        case ACTION_NO_SIG_READY:
            cout << "\033[1;33m";
        }
        cout << setw(ceil(log10((double)ACTION_NUM_SIGS))) << action.sig << "\033[0m"
             << " / Datetime: " << TRANSLATE_MQTT_DATETIME_TO_STRING(action.datetime) << endl;
        /* END */
#endif
    }
    /* END */

    /* MQTT Sensor Message Reception */
#ifdef TEST
    else if (msg_topic.find(MQTT_SENSOR_TEST_TOPIC) != msg_topic.npos)
#else
    else if (msg_topic.find(MQTT_SENSOR_TOPIC) != msg_topic.npos)
#endif
    {
#ifdef DEBUG
        /* Dump Sensor Message */
        debug_ofs_sensor << msg_root << endl;
        /* END */
#endif

        /* Get Sensor User ID & Device ID(currently only 1 device per user is allowed) */
        string user_id = msg_root[MQTT_SENSOR_MSG_KEYS.USER_ID].asString();
        string device_id = msg_root[MQTT_SENSOR_MSG_KEYS.DEVICE_ID].asString();
        if (global_user_data.find(user_id) == global_user_data.end())
        {
            global_user_data[user_id] = User(device_id);
        }
        /* END */

        /* Get Datetime */
        string datetime_str = msg_root[MQTT_SENSOR_MSG_KEYS.DATETIME].asString();
        MQTT_DATETIME datetime = TRANSLATE_MQTT_DATETIME_FROM_STRING(datetime_str);
        /* END */

#ifdef DEBUG
        /* Print Sensor Debug Message to Console */
        cout << "[DEBUG] Sensor Message Reception -> User ID: " << user_id << " / Device ID: " << device_id;
        cout << " / Datetime: " << TRANSLATE_MQTT_DATETIME_TO_STRING(datetime) << endl;
        /* END */
#endif

        /* Store Datetime if Acceleration Exceeds Threshold */
        Json::Value sensor_msg_data = msg_root[MQTT_SENSOR_MSG_KEYS.DATA];
        size_t sensor_msg_data_size = sensor_msg_data.size();
        for (Json::ValueIterator sensor_msg_data_iter = sensor_msg_data.begin(); sensor_msg_data_iter != sensor_msg_data.end(); sensor_msg_data_iter++)
        {
            /* Calculate Acceleration & Store If Excessive */
            string ax, ay, az;
            ax = (*sensor_msg_data_iter)[MQTT_SENSOR_MSG_KEYS.DATA_ELEM_KEYS.AX].asString();
            ay = (*sensor_msg_data_iter)[MQTT_SENSOR_MSG_KEYS.DATA_ELEM_KEYS.AY].asString();
            az = (*sensor_msg_data_iter)[MQTT_SENSOR_MSG_KEYS.DATA_ELEM_KEYS.AZ].asString();
            XYZ axyz(atof(ax.c_str()), atof(ay.c_str()), atof(az.c_str()));

            ACCEL accel;
            accel.mag = axyz.getL();
            int index = atoi((*sensor_msg_data_iter)[MQTT_SENSOR_MSG_KEYS.DATA_ELEM_KEYS.INDEX].asString().c_str());
            accel.datetime = ADD_MQTT_DATETIME_WITH_TIME(datetime, 0, 0, 0, (double)index / sensor_msg_data_size);
            global_user_data[user_id].storeAccelerationIfExcessive(accel);

#ifdef DEBUG
            /* Print Excessive Acceleration Debug Message to Console */
            if (accel.mag > ACCEL_MAG_HIGH_THRESHOLD)
            {
                cout << "[DEBUG] \033[1;31m"
                     << "Excessive Acceleration Detection\033[0m -> Magnitude: " << fixed << setprecision(DEBUG_ACCEL_MAG_STR_PRECISION)
                     << accel.mag << " / Datetime: " << TRANSLATE_MQTT_DATETIME_TO_STRING(accel.datetime) << "\033[0m" << endl;
            }
            else if (accel.mag > ACCEL_MAG_LOW_THRESHOLD)
            {
                cout << "[DEBUG] \033[1;34m"
                     << "Normal Acceleration Detection\033[0m -> Magnitude: " << fixed << setprecision(DEBUG_ACCEL_MAG_STR_PRECISION)
                     << accel.mag << " / Datetime: " << TRANSLATE_MQTT_DATETIME_TO_STRING(accel.datetime) << endl;
            }
            /* END */
#endif

            /* END */
        }
        /* END */

        /* Store Heart Rate */
        HEART_COND heart_cond;
        heart_cond.datetime = datetime;
        heart_cond.heart_rate = atoi(msg_root[MQTT_SENSOR_MSG_KEYS.HEART_RATE].asString().c_str());
#ifdef DEBUG
        cout << "[DEBUG] Current Heartrate is ";
        if(heart_cond.heart_rate > HEART_RATE_UPPER_THRESHOLD || (heart_cond.heart_rate < HEART_RATE_LOWER_THRESHOLD) && (heart_cond.heart_rate > HEART_RATE_MIN_THRESHOLD))
            cout << "\033[1;33m";
        else if(heart_cond.heart_rate <= HEART_RATE_MIN_THRESHOLD)
            cout << "\033[1;31m";
        cout << heart_cond.heart_rate << "\033[0mbpm." << endl; 
#endif
        global_user_data[user_id].storeAbnormalHeartCondition(heart_cond);
        /* END */
    }
    /* END */
}
