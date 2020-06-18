using namespace std;

#include <iomanip>
#include <iostream>
#include <sstream>

#include <sys/time.h>

#include "dc_mqtt.hpp"

string MQTT_TOPIC_WITH_SUBTOPICS(string topic, MQTT_SUBTOPIC_LIST subtopics)
{
    stringstream topic_ss;
    topic_ss << topic;
    if (topic_ss.str().back() != '/')
    {
        topic_ss << "/";
    }
    for (MQTT_SUBTOPIC_LIST::iterator subtopics_iter = subtopics.begin(); subtopics_iter != subtopics.end(); subtopics_iter++)
    {
        topic_ss << *subtopics_iter;
        if (subtopics_iter->compare("#"))
        {
            topic_ss << "/";
        }
    }
    return topic_ss.str();
}
string MQTT_ALL_TOPICS_RELATED_WITH(string topic)
{
    MQTT_SUBTOPIC_LIST subtopics;
    subtopics.push_back("#");
    return topic = MQTT_TOPIC_WITH_SUBTOPICS(topic, subtopics);
}

void TRANSLATE_MQTT_DATETIME(MQTT_DATETIME datetime, int &year, int &mon, int &day, int &hour, int &min, double &sec)
{
    unsigned long long time_int = datetime.time_int;
    year = time_int / 10000000000;
    mon = (time_int / 100000000) % 100;
    day = (time_int / 1000000) % 100;
    hour = (time_int / 10000) % 100;
    min = (time_int / 100) % 100;
    sec = datetime.time_dec + time_int % 100;
}
MQTT_DATETIME TRANSLATE_MQTT_DATETIME_FROM_STRING(string datetime_str)
{
    stringstream datetime_ss;
    datetime_ss << datetime_str;
    string datetime_int;
    getline(datetime_ss, datetime_int, '.');
    string datetime_dec;
    getline(datetime_ss, datetime_dec);
    datetime_dec.insert(0, "0.");

    MQTT_DATETIME datetime;
    datetime.time_int = strtoull(datetime_int.c_str(), NULL, 10);
    datetime.time_dec = atof(datetime_dec.c_str());
    return datetime;
}
string TRANSLATE_MQTT_DATETIME_TO_STRING(MQTT_DATETIME datetime)
{
    stringstream datetime_dec_ss;
    datetime_dec_ss << fixed << setprecision(MQTT_DATETIME_STR_PRECISION) << datetime.time_dec;
    datetime_dec_ss.seekg(1, datetime_dec_ss.beg);
    string datetime_dec;
    datetime_dec_ss >> datetime_dec;

    stringstream datetime_ss;
    datetime_ss << datetime.time_int;
    datetime_ss << datetime_dec;
    return datetime_ss.str();
}
MQTT_DATETIME CREATE_MQTT_DATETIME(int year, int mon, int day, int hour, int min, double sec)
{
    MQTT_DATETIME datetime;
    if (year < 0 || mon < 0 || day < 0 || hour < 0 || min < 0 || sec < 0)
    {
        cerr << "CREATE_MQTT_DATETIME(): invalid time format, new MQTT_DATETIME variable has been initialized as 0" << endl;
        datetime.time_int = 0;
        datetime.time_dec = 0.0;
    }
    else
    {
        stringstream datetime_int_ss;
        datetime_int_ss.fill('0');
        datetime_int_ss << year << setw(2) << mon << setw(2) << day << setw(2) << hour << setw(2) << min << setw(2) << (int)sec;

        datetime.time_int = strtoull(datetime_int_ss.str().c_str(), NULL, 10);
        datetime.time_dec = sec - (int)sec;
    }
    return datetime;
}
MQTT_DATETIME GET_CURRENT_MQTT_DATETIME()
{
    time_t cur_time = time(NULL);
    struct tm *cur_tm = localtime(&cur_time);
    struct timeval cur_tv;
    gettimeofday(&cur_tv, NULL);
    int year = cur_tm->tm_year + 1900;
    int mon = cur_tm->tm_mon + 1;
    int day = cur_tm->tm_mday;
    int hour = cur_tm->tm_hour;
    int min = cur_tm->tm_min;
    double sec = cur_tm->tm_sec + cur_tv.tv_usec * 0.000001;
    return CREATE_MQTT_DATETIME(year, mon, day, hour, min, sec);
}
MQTT_DATETIME ADD_MQTT_DATETIME_WITH_TIME(MQTT_DATETIME datetime, int days, int hours, int mins, double secs)
{
    int year, mon, day, hour, min;
    double sec;
    TRANSLATE_MQTT_DATETIME(datetime, year, mon, day, hour, min, sec);

    double total_sec = sec + secs;
    double res_sec = (int)total_sec % 60 + total_sec - (int)total_sec;

    int carry_min = (int)total_sec / 60;
    int total_min = min + mins;
    int res_min = (total_min + carry_min) % 60;

    int carry_hour = (total_min + carry_min) / 60;
    int total_hour = hour + hours;
    int res_hour = (total_hour + carry_hour) % 24;

    int carry_day = (total_hour + carry_hour) / 24;
    days += carry_day;
    int res_day = day;
    int res_mon = mon;
    int res_year = year;
    int day_offset;
    while (days)
    {
        switch (res_mon)
        {
        case 2:
            if (res_year % 4)
            {
                day_offset = 3;
            }
            else
            {
                if (res_year % 100)
                {
                    day_offset = 2;
                }
                else
                {
                    if (res_year % 400)
                    {
                        day_offset = 3;
                    }
                    else
                    {
                        day_offset = 2;
                    }
                }
            }
            break;
        case 4:
        case 6:
        case 9:
        case 11:
            day_offset = 1;
            break;
        default:
            day_offset = 0;
        }
        switch (res_mon)
        {
        case 13:
            res_mon = 1;
            res_year++;
            break;
        default:
            if (res_day + days <= 31 - day_offset)
            {
                res_day += days;
                days = 0;
            }
            else
            {
                days -= 32 - day_offset - res_day;
                res_day = 1;
                if (days < 28)
                {
                    res_day += days;
                    days = 0;
                }
                else
                {
                    res_day += 27;
                    days -= 27;
                }
            }
            res_mon++;
        }
    }
    return CREATE_MQTT_DATETIME(res_year, res_mon, res_day, res_hour, res_min, res_sec);
}
bool LATER_MQTT_DATETIME(MQTT_DATETIME first, MQTT_DATETIME second)
{
    if (first.time_int > second.time_int)
    {
        return true;
    }
    else if (first.time_int == second.time_int)
    {
        if (first.time_dec > second.time_dec)
        {
            return true;
        }
    }
    return false;
}
bool EQUAL_MQTT_DATETIME(MQTT_DATETIME first, MQTT_DATETIME second)
{
    if ((first.time_int == second.time_int) && (first.time_dec == second.time_dec))
    {
        return true;
    }
    return false;
}
