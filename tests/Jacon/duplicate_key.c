#include "jacon.h"
#include <stdio.h>

int main(void) {
    int err = JACON_OK;

    puts("Running test for duplicate name validation (valid)");
    Jacon_content valid_content = {0};
    Jacon_init_content(&valid_content);
    const char* valid_input = "{\"name\": 1, \"name_1\": 2}";
    err = Jacon_deserialize(&valid_content, valid_input);
    if (err != JACON_OK) {
        fprintf(stderr, "ERROR(%s:%d): unexpected invalid input\n", __FILE__, __LINE__);
    }
    
    puts("Running test for duplicate name validation (invalid)");
    Jacon_content invalid_content = {0};
    Jacon_init_content(&invalid_content);
    const char* invalid_input = "{\"name\": 1, \"name\": 2}";
    err = Jacon_deserialize(&invalid_content, invalid_input);
    if (err == JACON_OK) {
        fprintf(stderr, "ERROR(%s:%d): unexpected valid input\n", __FILE__, __LINE__);
    }
    return 0;
}