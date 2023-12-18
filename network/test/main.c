//
// Created by vyach on 19.11.2023.
//

#include <stdio.h>
#include "lib.h"

int main() {
    PhoneNumber phoneNumber = PHONE_NUMBER__INIT;
    phoneNumber.number = "1";
    phone_number__init(&phoneNumber);
    
    printf("Hello, world \n");
    return 0;
}
