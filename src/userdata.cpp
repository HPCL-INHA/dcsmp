using namespace std;

#ifdef DEBUG
#include <iostream>
#endif

#include <cstring>

#include "userdata.hpp"

User::User()
{
}
User::User(string device_id)
{
    setDeviceID(device_id);
}

void User::setDeviceID(string device_id)
{
    this->device_id = device_id;
}

HEART_PROB_ENUM User::checkAbnormalHeartCondition(HEART_COND heart_cond) const
{
    if (heart_cond.heart_rate >= HEART_RATE_UPPER_THRESHOLD)
    {
        return HEART_RATE_HIGH;
    }
    else if (heart_cond.heart_rate <= HEART_RATE_MIN_THRESHOLD)
    {
        return HEART_RATE_STOP;
    }
    else if (heart_cond.heart_rate <= HEART_RATE_LOWER_THRESHOLD)
    {
        return HEART_RATE_LOW;
    }
    return HEART_NORMAL;
}

std::string User::getDeviceID() const
{
    return device_id;
}

FALL_PROB_ENUM User::popFallCondition(MQTT_DATETIME &incident_datetime, deque<ACTION> &action_buffer_bak)
{
    if (!excess_accel_buffer.size())
    {
        memset(&incident_datetime, 0x00, sizeof(MQTT_DATETIME));
        action_buffer_bak = action_buffer;
        return FALL_NORMAL;
    }

#ifdef DEBUG
    cout << "[DEBUG] Starting Fall Check..." << endl;
#endif

    ACCEL cur_accel = excess_accel_buffer.front();
    bool ready_to_process = false;
    for (size_t check_index = cnt_prev_action + cnt_next_action; check_index < action_buffer.size(); check_index++)
    {
        ACTION cur_action = action_buffer[check_index];
        if (LATER_MQTT_DATETIME(cur_action.datetime, cur_accel.datetime))
        {
            if (LATER_MQTT_DATETIME(ADD_MQTT_DATETIME_WITH_TIME(cur_accel.datetime, 0, 0, 0, ACTION_BUFFER_LOOKAHEAD_TIME_BOUND_S), cur_action.datetime))
            {
                cnt_next_action++;
            }
            else
            {
                ready_to_process = true;
                break;
            }
        }
        else
        {
            if (LATER_MQTT_DATETIME(ADD_MQTT_DATETIME_WITH_TIME(cur_action.datetime, 0, 0, 0, ACTION_BUFFER_LOOKBEHIND_TIME_BOUND_S), cur_accel.datetime))
            {
                cnt_prev_action++;
            }
            else
            {
                action_buffer.pop_front();
                popped_past_action = true;
                check_index--;
            }
        }
    }

    if (ready_to_process)
    {
        excess_accel_buffer.pop_front();
        ready_to_process = false;

        size_t num_predicted_action = 0, num_next_invalid_action = 0, num_prev_invalid_action = 0;
        for (size_t index = 0; index < cnt_next_action; index++)
        {
            switch (action_buffer[cnt_prev_action + index].sig) // After falling
            {
            case ACTION_NO_SIG:
            case ACTION_NO_SIG_READY:
                num_next_invalid_action++;
            case ACTION_SITTING_FLOOR_TO_LYING:
            case ACTION_SITTING_FLOOR_TO_STANDING:
            case ACTION_LYING_TO_SITTING_FLOOR:
            case ACTION_LYING:
            case ACTION_STANDING_TO_SITTING_FLOOR:
                num_predicted_action++;
            }
        }
        for (size_t index = 0; index < cnt_prev_action; index++) // Before falling
        {
            switch (action_buffer[index].sig)
            {
            case ACTION_NO_SIG:
            case ACTION_NO_SIG_READY:
                num_prev_invalid_action++;
                break;
            case ACTION_WALKING:
            case ACTION_STANDING:
            case ACTION_STANDING_TO_SITTING_CHAIR:
            case ACTION_SITTING_CHAIR:
            case ACTION_SITTING_CHAIR_TO_STANDING:
            case ACTION_SITTING_FLOOR:
                num_predicted_action++;
            }
        }

        size_t num_checked_action = cnt_prev_action + cnt_next_action;
        double pred_true_ratio = (double)num_predicted_action / num_checked_action;
        bool enough_next_actions = cnt_next_action >= ACTION_BUFFER_LOOKAHEAD_MIN_CNT, enough_prev_actions = cnt_prev_action >= ACTION_BUFFER_LOOKBEHIND_MIN_CNT,
             excess_next_invalid_action = (double)num_next_invalid_action / num_checked_action >= FALL_NEXT_INVALID_ACTION_THRESHOLD,
             excess_prev_invalid_action = (double)num_prev_invalid_action / num_checked_action >= FALL_PREV_INVALID_ACTION_THRESHOLD;

#ifdef DEBUG
        /* Print Falling Check Debug Message to Console */
        cout << boolalpha;
        cout << "[DEBUG] Fall Action Prediction Hit Ratio: ";
        if (pred_true_ratio > FALL_PRED_TRUE_RATIO_THRESHOLD)
        {
            cout << "\033[1;31m";
        }
        else if (pred_true_ratio > FALL_PRED_AMBIGUOUS_RATIO_THRESHOLD)
        {
            cout << "\033[1;33m";
        }
        else
        {
            cout << "\033[1;32m";
        }
        cout << pred_true_ratio << "\033[0m" << endl;
        cout << "[DEBUG] Have Popped Past Action : ";
        if (popped_past_action)
        {
            cout << "\033[1;32m";
        }
        else
        {
            cout << "\033[1;33m";
        }
        cout << boolalpha << popped_past_action << noboolalpha << "\033[0m" << endl;
        cout << "[DEBUG] Next Action Count: ";
        if (enough_next_actions)
        {
            cout << "\033[1;32m";
        }
        else
        {
            cout << "\033[1;33m";
        }
        cout << cnt_next_action << "\033[0m" << endl;
        cout << "[DEBUG] Previous Action Count: ";
        if (enough_prev_actions)
        {
            cout << "\033[1;32m";
        }
        else
        {
            cout << "\033[1;33m";
        }
        cout << cnt_prev_action << "\033[0m" << endl;
        cout << "[DEBUG] Number of Next Invalid Action: ";
        if (excess_next_invalid_action)
        {
            cout << "\033[1;33m";
        }
        else
        {
            cout << "\033[1;32m";
        }
        cout << num_next_invalid_action << "\033[0m" << endl;
        cout << "[DEBUG] Number of Previous Invalid Action: ";
        if (excess_prev_invalid_action)
        {
            cout << "\033[1;33m";
        }
        else
        {
            cout << "\033[1;32m";
        }
        cout << num_prev_invalid_action << "\033[0m" << endl;
        /* END */
#endif

        cnt_next_action = 0;
        cnt_prev_action = 0;
        action_buffer_bak = action_buffer;
        incident_datetime = cur_accel.datetime;

#ifdef DEBUG
        cout << "[DEBUG] Fall Check: ";
#endif

        if (!popped_past_action || !enough_next_actions || !enough_prev_actions || excess_next_invalid_action || excess_prev_invalid_action)
        {
#ifdef DEBUG
            cout << "\033[1;33m"
                 << "FALL_TRUE" // 수정 필요
                 << "\033[0m" << endl;
#endif

            return FALL_TRUE; // 수정 필요
        }
        if (pred_true_ratio >= FALL_PRED_TRUE_RATIO_THRESHOLD)
        {
#ifdef DEBUG
            cout << "\033[1;31m"
                 << "FALL_TRUE"
                 << "\033[0m" << endl;
#endif

            return FALL_TRUE;
        }
        if (pred_true_ratio >= FALL_PRED_AMBIGUOUS_RATIO_THRESHOLD)
        {
#ifdef DEBUG
            cout << "\033[1;33m"
                 << "FALL_TRUE" // 수정 필요
                 << "\033[0m" << endl;
#endif

            return FALL_TRUE; // 수정 필요
        }

#ifdef DEBUG
        cout << "\033[1;32m"
             << "FALL_NORMAL"
             << "\033[0m" << endl;
#endif

        return FALL_NORMAL;
    }

#ifdef DEBUG
    cout << "[DEBUG] Fall Check: "
         << "\033[1;34m"
         << "FALL_UNDETERMINED"
         << "\033[0m" << endl;
#endif

    return FALL_UNDETERMINED;
}
bool User::testNoRecentMovement()
{
    if (cnt_low_accel > ACCEL_NUM_EXPECT_ENTRIES * ACCEL_NO_MOVE_TRIG_DURATION_S)
    {
        cnt_low_accel = 0;
        return true;
    }
    return false;
}
void User::storeAccelerationIfExcessive(ACCEL accel)
{
    if (accel.mag > ACCEL_MAG_HIGH_THRESHOLD)
    {
        excess_accel_buffer.push_back(accel);
        if (excess_accel_buffer.size() > ACCEL_EXCESS_BUFFER_SIZE_LIM)
        {
            excess_accel_buffer.pop_front();
        }
    }

    if (accel.mag < ACCEL_MAG_LOW_THRESHOLD)
    {
        cnt_low_accel++;
    }
    else
    {
        cnt_low_accel = 0;
    }
}
void User::storeActionIntoBuffer(ACTION action)
{
    action_buffer.push_back(action);
    if (action_buffer.size() > ACTION_BUFFER_SIZE_LIM)
    {
        action_buffer.pop_front();
    }
}

HEART_PROB_ENUM User::popAbnormalHeartCondition(MQTT_DATETIME &incident_datetime)
{
    if (!abnorm_heart_conds.size())
    {
        memset(&incident_datetime, 0x00, sizeof(MQTT_DATETIME));
        return HEART_NORMAL;
    }

    HEART_COND cur_heart_cond = abnorm_heart_conds.front();
    abnorm_heart_conds.pop_front();
    incident_datetime = cur_heart_cond.datetime;
    return checkAbnormalHeartCondition(cur_heart_cond);
}
void User::storeAbnormalHeartCondition(HEART_COND heart_cond)
{
    if (checkAbnormalHeartCondition(heart_cond))
    {
        abnorm_heart_conds.push_back(heart_cond);
    }
}
