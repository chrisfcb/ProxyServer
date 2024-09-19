/*
 * This file implements two functions that read XML and binary information from a buffer,
 * respectively, and return pointers to Record or NULL.
 *
 * *** YOU MUST IMPLEMENT THESE FUNCTIONS ***
 *
 * The parameters and return values of the existing functions must not be changed.
 * You can add function, definition etc. as required.
 */
#include "recordFromFormat.h"

char* get_attribute_value(char* buffer, char* attribute_name) {
    char* start = strstr(buffer, attribute_name);
    if (start == NULL) {
        return NULL;
    }

    start += strlen(attribute_name) + 2;
    char* end = strchr(start, '/') - 2;

    int len = end - start;
    char* data = malloc(len + 1);
    strncpy(data, start, len);
    data[len] = '\0';
    return data;
}

Record* XMLtoRecord( char* buffer, int bufSize, int* bytesread )
{
    Record *record = newRecord();
    initRecord(record);
    char *attributeValue;
    unsigned long UlVal;

    attributeValue = get_attribute_value(buffer, "source");
    if (attributeValue != NULL) {
        setSource(record, attributeValue[0]); 
        free(attributeValue);
    }

    attributeValue = get_attribute_value(buffer, "dest");
    if (attributeValue != NULL) {
        setDest(record, attributeValue[0]); 
        free(attributeValue);
    }


    attributeValue = get_attribute_value(buffer, "username");
    if (attributeValue != NULL) {
        setUsername(record, attributeValue); 
        free(attributeValue);
    }


    attributeValue = get_attribute_value(buffer, "id");
    if (attributeValue != NULL) {
        UlVal = strtoul(attributeValue, NULL, 10);
        setId(record, (uint32_t) UlVal); 
        free(attributeValue);
    }


    attributeValue = get_attribute_value(buffer, "group");
    if (attributeValue != NULL) {
        UlVal = strtoul(attributeValue, NULL, 10);
        setGroup(record, (uint32_t) UlVal); 
        free(attributeValue);
    }

    attributeValue = get_attribute_value(buffer, "semester");
    if (attributeValue != NULL) {
        UlVal = strtoul(attributeValue, NULL, 10);
        setSemester(record, (uint8_t) UlVal); 
        free(attributeValue);
    }

    Grade grade;
    attributeValue = get_attribute_value(buffer, "grade");
        if (attributeValue != NULL) {
            if (strcmp(attributeValue, "None") == 0) {
                grade = Grade_None;
            } else if (strcmp(attributeValue, "Bachelor") == 0) {
                grade = Grade_Bachelor;
            } else if (strcmp(attributeValue, "Master") == 0) {
                grade = Grade_Master;
            } else if (strcmp(attributeValue, "PhD") == 0) {
                grade = Grade_PhD;
            } else {
                return NULL;
            }

            setGrade(record, grade); 
            free(attributeValue);
    }

    attributeValue = get_attribute_value(buffer, "courses");
    if (attributeValue != NULL) {
        char* start = strstr(buffer, "courses");
        if (start == NULL) {
            return NULL;
        }

        char attr_name[strlen("courses") + 2];
        strcpy(attr_name, "/");
        strcat(attr_name, "courses");

        start += strlen("courses") + 1; // +1 to skip over '="'
        char* end = strstr(buffer, attr_name) - 1;

        int len = end - start;
        char* data = malloc(len + 1);
        strncpy(data, start, len);
        data[len] = '\0';

        for (int i = 0; data[i] != '\0'; i++) {
            char course[7] = { 0 };
            if (data[i] == '=') {
                char* start = strchr(data, data[i]) + 2;
                data[i] = '|';
                strncpy(course, start, 6);
                if (strcmp(course, "IN1000") == 0) {
                    setCourse(record, Course_IN1000);
                } else if (strcmp(course, "IN1010") == 0) {
                    setCourse(record, Course_IN1010);
                } else if (strcmp(course, "IN1020") == 0) {
                    setCourse(record, Course_IN1020);
                } else if (strcmp(course, "IN1030") == 0) {
                    setCourse(record, Course_IN1030);
                } else if (strcmp(course, "IN1050") == 0) {
                    setCourse(record, Course_IN1050);
                } else if (strcmp(course, "IN1060") == 0) {
                    setCourse(record, Course_IN1060);
                } else if (strcmp(course, "IN1080") == 0) {
                    setCourse(record, Course_IN1080);
                } else if (strcmp(course, "IN1140") == 0) {
                    setCourse(record, Course_IN1140);
                } else if (strcmp(course, "IN1150") == 0) {
                    setCourse(record, Course_IN1150);
                } else if (strcmp(course, "IN1900") == 0) {
                    setCourse(record, Course_IN1900);
                } else if (strcmp(course, "IN1910") == 0) {
                    setCourse(record, Course_IN1910);
                } else {
                    return NULL;
                }
            }
        }
        free(data);
    }

    if(record->has_dest) {
        return record;
    }

    deleteRecord(record);
    return NULL;
}

Record* BinaryToRecord( char* buffer, int bufSize, int* bytesread )
{
    Record *record = newRecord();
    initRecord(record);
    uint32_t usernameLength = 0;
    int bytes_read = 0;

    uint8_t flags = buffer[0];
    bytes_read += 1;

    if (flags & FLAG_SRC) {
        setSource(record, buffer[bytes_read]);
        bytes_read += 1;
    }
    
    if (flags & FLAG_DST) {
        setDest(record, buffer[bytes_read]);
        bytes_read += 1;
    }

    if (flags & FLAG_USERNAME) {
        memcpy(&usernameLength, buffer + bytes_read, sizeof(uint32_t));
        usernameLength = ntohl(usernameLength);
        bytes_read += sizeof(uint32_t);
        char* username = strndup(buffer + bytes_read, usernameLength);
        username[usernameLength] = '\0';
        bytes_read += usernameLength;

        setUsername(record, username);
    }

    if (flags & FLAG_ID) {
        uint32_t *id = malloc(sizeof(uint32_t));
        memcpy(id, buffer + bytes_read, sizeof(uint32_t));
        setId(record, *id);
        bytes_read += sizeof(uint32_t);
        free(id);
    }

    if (flags & FLAG_GROUP) {
        uint32_t *group = malloc(sizeof(uint32_t));
        memcpy(group, buffer + bytes_read, sizeof(uint32_t));
        setGroup(record, *group);
        bytes_read += sizeof(uint32_t);
        free(group);
    }

    if (flags & FLAG_SEMESTER) {
        setSemester(record, buffer[bytes_read]);
        bytes_read += 1;
    }

    if (flags & FLAG_GRADE) {
        uint8_t gradeVal = buffer[bytes_read];
        if (gradeVal == 0) {
            setGrade(record, Grade_None);
        } else if (gradeVal == 1) {
            setGrade(record, Grade_Bachelor);
        } else if (gradeVal == 2) {
            setGrade(record, Grade_Master);
        } else if (gradeVal == 3) {
            setGrade(record, Grade_PhD);
        }
        bytes_read += 1;
    }

    if (flags & FLAG_COURSES) {
        int16_t courses = buffer[bytes_read];
        if ((courses & 1 << 0) != 0) {
            setCourse(record, Course_IN1000);
        } else if ((courses & 1 << 1) != 0) {
            setCourse(record, Course_IN1010);
        } else if ((courses & 1 << 2) != 0) {
            setCourse(record, Course_IN1020);
        } else if ((courses & 1 << 3) != 0) {
            setCourse(record, Course_IN1030);
        } else if ((courses & 1 << 4) != 0) {
            setCourse(record, Course_IN1050);
        } else if ((courses & 1 << 5) != 0) {
            setCourse(record, Course_IN1060);
        } else if ((courses & 1 << 6) != 0) {
            setCourse(record, Course_IN1080);
        } else if ((courses & 1 << 7) != 0) {
            setCourse(record, Course_IN1140);
        } else if ((courses & 1 << 8) != 0) {
            setCourse(record, Course_IN1150);
        } else if ((courses & 1 << 9) != 0) {
            setCourse(record, Course_IN1900);
        } else if ((courses & 1 << 10) != 0) {
            setCourse(record, Course_IN1910);
        } else {
            return NULL;
        }
    }

    if(record->has_dest) {
        return record;
    }

    deleteRecord(record);
    return NULL;
}

