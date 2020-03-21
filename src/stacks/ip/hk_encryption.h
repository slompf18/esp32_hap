#pragma once

#include "../include/hk_mem.h"
#include "hk_pair_verify_keys.h"

#include <esp_err.h>

typedef struct hk_encryption_data
{
    int received_frame_count;
    int sent_frame_count;
} hk_encryption_data_t;

esp_err_t hk_session_security_decrypt_frames(hk_encryption_data_t *encryption_data, hk_pair_verify_keys_t *keys, hk_mem *in, hk_mem *out);
esp_err_t hk_session_security_encrypt_frames(hk_encryption_data_t *encryption_data, hk_pair_verify_keys_t *keys, hk_mem *in, hk_mem *out);
hk_encryption_data_t* hk_encryption_data_init();
void hk_encryption_data_free(hk_encryption_data_t* data);