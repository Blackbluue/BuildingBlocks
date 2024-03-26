#include "buildingblocks.h"
#include <stddef.h>

/* PUBLIC FUNCTIONS */

void set_err(int *err, int value) {
    if (err != NULL) {
        *err = value;
    }
}
