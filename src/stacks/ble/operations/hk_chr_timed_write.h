
#pragma once

#include "../../../include/hk_mem.h" 

#include "../hk_connection.h"
#include "../hk_chr.h"

esp_err_t hk_chr_timed_write(hk_transaction_t *transaction, hk_chr_t *chr);
esp_err_t hk_chr_execute_write(hk_connection_t *connection, hk_transaction_t *transaction, hk_chr_t *chr);