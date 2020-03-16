#include "hk_advertising.h"

#include <services/gap/ble_svc_gap.h>

#include "../../utils/hk_logging.h"
#include "../../utils/hk_util.h"
#include "../../common/hk_pairings_store.h"

static uint8_t hk_advertising_own_addr_type;
const char *hk_advertising_name; // todo: move to config
size_t hk_advertising_category;  // todo: move to config, type to char

/**
 * The nimble host executes this callback when a GAP event occurs.  The
 * application associates a GAP event callback with each connection that forms.
 * bleprph uses the same callback for all connections.
 *
 * @param event                 The type of event being signalled.
 * @param ctxt                  Various information pertaining to the event.
 * @param arg                   Application-specified argument; unused by
 *                                  bleprph.
 *
 * @return                      0 if the application successfully handled the
 *                                  event; nonzero on failure.  The semantics
 *                                  of the return code is specific to the
 *                                  particular GAP event being signalled.
 */
static int hk_advertising_gap_event(struct ble_gap_event *event, void *arg)
{
    //HK_LOGI("event");
    struct ble_gap_conn_desc desc;
    int rc;

    switch (event->type)
    {
    case BLE_GAP_EVENT_CONNECT:
        /* A new connection was established or a connection attempt failed. */
        HK_LOGI("connection %s; status=%d ",
                event->connect.status == 0 ? "established" : "failed",
                event->connect.status);
        if (event->connect.status == 0)
        {
            rc = ble_gap_conn_find(event->connect.conn_handle, &desc);
        }

        if (event->connect.status != 0)
        {
            /* Connection failed; resume advertising. */
            hk_advertising_start_advertising(hk_advertising_own_addr_type);
        }
        rc = 0;
        break;
    case BLE_GAP_EVENT_DISCONNECT:
        HK_LOGI("disconnect; reason=%d ", event->disconnect.reason);
        hk_advertising_start_advertising(hk_advertising_own_addr_type);
        rc = 0;
        break;
    case BLE_GAP_EVENT_CONN_UPDATE:
        HK_LOGI("connection updated; status=%d ", event->conn_update.status);
        rc = 0;
        break;
    case BLE_GAP_EVENT_ADV_COMPLETE:
        HK_LOGI("advertise complete; reason=%d", event->adv_complete.reason);
        hk_advertising_start_advertising(hk_advertising_own_addr_type);
        rc = 0;
        break;
    case BLE_GAP_EVENT_ENC_CHANGE:
        HK_LOGI("encryption change event; status=%d ", event->enc_change.status);
        rc = 0;
        break;
    case BLE_GAP_EVENT_SUBSCRIBE:
        HK_LOGI("subscribe event; conn_handle=%d attr_handle=%d "
                "reason=%d prevn=%d curn=%d previ=%d curi=%d\n",
                event->subscribe.conn_handle,
                event->subscribe.attr_handle,
                event->subscribe.reason,
                event->subscribe.prev_notify,
                event->subscribe.cur_notify,
                event->subscribe.prev_indicate,
                event->subscribe.cur_indicate);
        rc = 0;
        break;
    case BLE_GAP_EVENT_MTU:
        HK_LOGI("mtu update event; conn_handle=%d cid=%d mtu=%d",
                event->mtu.conn_handle,
                event->mtu.channel_id,
                event->mtu.value);
        rc = 0;
        break;
    case BLE_GAP_EVENT_REPEAT_PAIRING:
        HK_LOGI("Repeat pairing");
        rc = 0;
        break;
    case BLE_GAP_EVENT_PASSKEY_ACTION:
        HK_LOGI("PASSKEY_ACTION_EVENT started");
        rc = 0;
        break;
    }

    return rc;
}

void hk_advertising_set_address(uint8_t own_addr_type)
{
    hk_advertising_own_addr_type = own_addr_type;
}

void hk_advertising_start_advertising()
{
    HK_LOGD("Starting advertising.");
    int res;

    uint8_t device_id[6] = {0, 0, 0, 0, 0, 0};
    if(hk_util_get_accessory_id(device_id)){
        HK_LOGE("Could not start advertising, because getting device id failed.");
        return;
    }

    uint8_t manufacturer_data[17];
    manufacturer_data[0] = 0x4c;                   // company id
    manufacturer_data[1] = 0x00;                   // company id
    manufacturer_data[2] = 0x06;                   // type
    manufacturer_data[3] = 0xcd;                   // subtype and length
    manufacturer_data[4] = 0x01; // todo: this was uncommented for debugging: hk_pairings_store_has_pairing() ? 0x00 : 0x01;                   // pairing status flat
    manufacturer_data[5] = device_id[0];                   // device id
    manufacturer_data[6] = device_id[1];                   // device id
    manufacturer_data[7] = device_id[2];                   // device id
    manufacturer_data[8] = device_id[3];                   // device id
    manufacturer_data[9] = device_id[4];                   // device id
    manufacturer_data[10] = device_id[5];                  // device id
    manufacturer_data[11] = (char)hk_advertising_category; // accessory category identifier
    manufacturer_data[12] = 0x00;                  // accessory category identifier
    manufacturer_data[13] = 0x01;                  // global state number // todo: should be changed. See chapter 7.4.1.8
    manufacturer_data[14] = 0x00;                  // global state number
    manufacturer_data[15] = 0x01;                  // configuration number // todo: make configurable
    manufacturer_data[16] = 0x02;                  // HAP BLE version

    struct ble_hs_adv_fields fields;
    memset(&fields, 0, sizeof fields);
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP; // Discoverability in forthcoming advertisement (general) // BLE-only (BR/EDR unsupported)
    fields.name = (uint8_t *)hk_advertising_name;
    fields.name_len = strlen(hk_advertising_name);
    fields.name_is_complete = 1;
    fields.mfg_data = manufacturer_data;
    fields.mfg_data_len = sizeof(manufacturer_data);

    res = ble_gap_adv_set_fields(&fields);
    if (res != 0)
    {
        HK_LOGE("Could not start advertising, because fields could not be set. Errorcode: %d", res);
        return;
    }

    /* Begin advertising. */
    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof adv_params);
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    //adv_params.itvl_min = 20;
    res = ble_gap_adv_start(hk_advertising_own_addr_type, NULL, BLE_HS_FOREVER, &adv_params, hk_advertising_gap_event, NULL);
    if (res != 0)
    {
        HK_LOGE("Could not start advertising. Errorcode: %d", res);
        return;
    }
}

void hk_advertising_stop_advertising()
{
    int res = ble_gap_adv_stop();
    if (res != 0)
    {
        HK_LOGE("Could not stop advertising. Errorcode: %d", res);
    }
}

void hk_advertising_init(const char *name, size_t category, size_t config_version)
{
    HK_LOGD("Initializing GAP.");
    ble_svc_gap_init();
    hk_advertising_name = name;
    hk_advertising_category = category;
    int res = ble_svc_gap_device_name_set(name);
    if (res != ESP_OK)
    {
        HK_LOGE("Error setting name for advertising.");
        return;
    }
}

void hk_advertising_update_paired()
{
    //hk_advertising_stop_advertising();
    //hk_advertising_start_advertising();
}