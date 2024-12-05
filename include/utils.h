#pragma once

#include <stdlib.h>
#include "../include/client.h"
#include "../include/darray.h"

int ordered_insert(Client *client, Darray *clients_arr);

void dealoc_client(void *client);

