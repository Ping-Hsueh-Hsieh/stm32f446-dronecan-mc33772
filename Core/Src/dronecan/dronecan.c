/*
  A simple example DroneCAN node implementing a smart battery

  This example implements 6 features:

   - announces on the bus using NodeStatus at 1Hz
   - answers GetNodeInfo requests
   - implements dynamic node allocation
   - sends BatteryInfo messages with synthetic data
   - a parameter server for reading and writing node parameters

  This example uses socketcan or multicast UDP on Linux for CAN transport

  Example usage:
     ./battery_node vcan0
     ./battery_node mcast:0
*/
/*
 This example application is distributed under the terms of CC0 (public domain dedication).
 More info: https://creativecommons.org/publicdomain/zero/1.0/
*/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#include "canard_stm32.h"
#include "stm32f4xx_hal.h"
#endif

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "can.h"
#include "canard.h"
#include "dronecan.h"
#include "rte.h"
#include "util.h"

// include the headers for the generated DroneCAN messages from the
// dronecan_dsdlc compiler
#include "dronecan_msgs.h"

/*
  libcanard library instance and a memory pool for it to use
 */
static CanardInstance canard;
static uint8_t memory_pool[1024];

/*
  in this example we will use dynamic node allocation if MY_NODE_ID is zero
 */
#define MY_NODE_ID 69

/*
  our preferred node ID if nobody else has it
 */
#define PREFERRED_NODE_ID 73

#define BATTERY_MANUFACTURER_NAME "LUL Battery"

/*
  keep the state of the battery
 */

/*
  state of user settings. This will be saved in settings.dat. On a
  real device a better storage system will be needed
  For simplicity we store all parameters as floats in this example
 */
static struct
{
    float can_node;
    float battery_index;
    float telem_rate;
} settings;

/*
  a set of parameters to present to the user. In this example we don't
  actually save parameters, this is just to show how to handle the
  parameter protocol
 */
static struct parameter
{
    char* name;
    enum uavcan_protocol_param_Value_type_t type;
    float* value;
    float min_value;
    float max_value;
} parameters[] = {
    // add any parameters you want users to be able to set
    {"CAN_NODE", UAVCAN_PROTOCOL_PARAM_VALUE_INTEGER_VALUE, &settings.can_node, 0, 127},             // CAN node ID
    {"BATTERY_INDEX", UAVCAN_PROTOCOL_PARAM_VALUE_INTEGER_VALUE, &settings.battery_index, 0, 32},    // index in RawCommand
    {"TELEM_RATE", UAVCAN_PROTOCOL_PARAM_VALUE_INTEGER_VALUE, &settings.telem_rate, 0, 32},          // index in RawCommand
};

// some convenience macros
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define ARRAY_SIZE(x) (sizeof(x) / sizeof(x[0]))

/*
  hold our node status as a static variable. It will be updated on any errors
 */
static struct uavcan_protocol_NodeStatus node_status;

/*
  get monotonic time in milliseconds since startup
 */
static uint32_t millis32(void)
{
    return HAL_GetTick();
}

/*
  get a 64 bit monotonic timestamp in microseconds since start. This
  is platform specific
 */
static uint64_t micros64(void)
{
    return millis32() * 1000U;
}

/*
  get a 16 byte unique ID for this node, this should be based on the CPU unique ID or other unique ID
 */
static void getUniqueID(uint8_t id[16])
{
    char* my_id = "xxxxxxxxxxxxxxxx";
    memcpy(id, my_id, 16);
}

/*
  save all settings
 */
static void save_settings(void)
{
    TODO("[ERROR] save_settings");
}

/*
  load all settings
 */
static void load_settings(void)
{
    TODO("[ERROR] load_settings");
}

/*
  handle parameter GetSet request
 */
static void handle_param_GetSet(CanardInstance* ins, CanardRxTransfer* transfer)
{
    struct uavcan_protocol_param_GetSetRequest req;
    if (uavcan_protocol_param_GetSetRequest_decode(transfer, &req)) {
        return;
    }

    struct parameter* p = NULL;
    if (req.name.len != 0) {
        for (uint16_t i = 0; i < ARRAY_SIZE(parameters); i++) {
            if (req.name.len == strlen(parameters[i].name) && strncmp((const char*)req.name.data, parameters[i].name, req.name.len) == 0) {
                p = &parameters[i];
                break;
            }
        }
    } else if (req.index < ARRAY_SIZE(parameters)) {
        p = &parameters[req.index];
    }
    if (p != NULL && req.name.len != 0 && req.value.union_tag != UAVCAN_PROTOCOL_PARAM_VALUE_EMPTY) {
        /*
          this is a parameter set command. The implementation can
          either choose to store the value in a persistent manner
          immediately or can instead store it in memory and save to permanent storage on a
         */
        switch (p->type) {
        case UAVCAN_PROTOCOL_PARAM_VALUE_INTEGER_VALUE: *p->value = req.value.integer_value; break;
        case UAVCAN_PROTOCOL_PARAM_VALUE_BOOLEAN_VALUE: *p->value = req.value.boolean_value; break;
        case UAVCAN_PROTOCOL_PARAM_VALUE_REAL_VALUE: *p->value = req.value.real_value; break;
        default: return;
        }
        save_settings();
    }

    /*
      for both set and get we reply with the current value
     */
    struct uavcan_protocol_param_GetSetResponse pkt;
    memset(&pkt, 0, sizeof(pkt));

    if (p != NULL) {
        pkt.value.union_tag = p->type;
        switch (p->type) {
        case UAVCAN_PROTOCOL_PARAM_VALUE_INTEGER_VALUE: pkt.value.integer_value = *p->value; break;
        case UAVCAN_PROTOCOL_PARAM_VALUE_BOOLEAN_VALUE: pkt.value.integer_value = *p->value; break;
        case UAVCAN_PROTOCOL_PARAM_VALUE_REAL_VALUE: pkt.value.real_value = *p->value; break;
        default: return;
        }
        pkt.name.len = strlen(p->name);
        strcpy((char*)pkt.name.data, p->name);
    }

    uint8_t buffer[UAVCAN_PROTOCOL_PARAM_GETSET_RESPONSE_MAX_SIZE];
    uint16_t total_size = uavcan_protocol_param_GetSetResponse_encode(&pkt, buffer);

    canardRequestOrRespond(ins, transfer->source_node_id, UAVCAN_PROTOCOL_PARAM_GETSET_SIGNATURE, UAVCAN_PROTOCOL_PARAM_GETSET_ID, &transfer->transfer_id, transfer->priority,
                           CanardResponse, &buffer[0], total_size);
}

/*
  handle parameter executeopcode request
 */
static void handle_param_ExecuteOpcode(CanardInstance* ins, CanardRxTransfer* transfer)
{
    struct uavcan_protocol_param_ExecuteOpcodeRequest req;
    if (uavcan_protocol_param_ExecuteOpcodeRequest_decode(transfer, &req)) {
        return;
    }
    if (req.opcode == UAVCAN_PROTOCOL_PARAM_EXECUTEOPCODE_REQUEST_OPCODE_ERASE) {
        // here is where you would reset all parameters to defaults
    }
    if (req.opcode == UAVCAN_PROTOCOL_PARAM_EXECUTEOPCODE_REQUEST_OPCODE_SAVE) {
        // here is where you would save all the changed parameters to permanent storage
    }

    struct uavcan_protocol_param_ExecuteOpcodeResponse pkt;
    memset(&pkt, 0, sizeof(pkt));

    pkt.ok = true;

    uint8_t buffer[UAVCAN_PROTOCOL_PARAM_EXECUTEOPCODE_RESPONSE_MAX_SIZE];
    uint16_t total_size = uavcan_protocol_param_ExecuteOpcodeResponse_encode(&pkt, buffer);

    canardRequestOrRespond(ins, transfer->source_node_id, UAVCAN_PROTOCOL_PARAM_EXECUTEOPCODE_SIGNATURE, UAVCAN_PROTOCOL_PARAM_EXECUTEOPCODE_ID, &transfer->transfer_id,
                           transfer->priority, CanardResponse, &buffer[0], total_size);
}

/*
  handle RestartNode request
 */
static void handle_RestartNode(CanardInstance* ins, CanardRxTransfer* transfer)
{
    (void)ins;
    (void)transfer;
    // the battery node should reboot now!
    printf("Rebooting!!!\n");
    exit(0);
}

/*
  handle a GetNodeInfo request
*/
static void handle_GetNodeInfo(CanardInstance* ins, CanardRxTransfer* transfer)
{
    printf("GetNodeInfo request from %d\n", transfer->source_node_id);

    uint8_t buffer[UAVCAN_PROTOCOL_GETNODEINFO_RESPONSE_MAX_SIZE];
    struct uavcan_protocol_GetNodeInfoResponse pkt;

    memset(&pkt, 0, sizeof(pkt));

    node_status.uptime_sec = millis32() / 1000U;
    pkt.status = node_status;

    // fill in your major and minor firmware version
    pkt.software_version.major = 4;
    pkt.software_version.minor = 20;
    pkt.software_version.optional_field_flags = 0;
    pkt.software_version.vcs_commit = 0;    // should put git hash in here

    // should fill in hardware version
    pkt.hardware_version.major = 6;
    pkt.hardware_version.minor = 9;

    getUniqueID(pkt.hardware_version.unique_id);

    strncpy((char*)pkt.name.data, "ExampleBatteryNode", sizeof(pkt.name.data));
    pkt.name.len = strnlen((char*)pkt.name.data, sizeof(pkt.name.data));

    uint16_t total_size = uavcan_protocol_GetNodeInfoResponse_encode(&pkt, buffer);

    canardRequestOrRespond(ins, transfer->source_node_id, UAVCAN_PROTOCOL_GETNODEINFO_SIGNATURE, UAVCAN_PROTOCOL_GETNODEINFO_ID, &transfer->transfer_id, transfer->priority,
                           CanardResponse, &buffer[0], total_size);
}

/*
  data for dynamic node allocation process
 */
static struct
{
    uint32_t send_next_node_id_allocation_request_at_ms;
    uint32_t node_id_allocation_unique_id_offset;
} DNA;

/*
  handle a DNA allocation packet
 */
static void handle_DNA_Allocation(CanardInstance* ins, CanardRxTransfer* transfer)
{
    if (canardGetLocalNodeID(&canard) != CANARD_BROADCAST_NODE_ID) {
        // already allocated
        return;
    }

    // Rule C - updating the randomized time interval
    DNA.send_next_node_id_allocation_request_at_ms =
        millis32() + UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MIN_REQUEST_PERIOD_MS + (random() % UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MAX_FOLLOWUP_DELAY_MS);

    if (transfer->source_node_id == CANARD_BROADCAST_NODE_ID) {
        printf("Allocation request from another allocatee\n");
        DNA.node_id_allocation_unique_id_offset = 0;
        return;
    }

    // Copying the unique ID from the message
    struct uavcan_protocol_dynamic_node_id_Allocation msg;

    uavcan_protocol_dynamic_node_id_Allocation_decode(transfer, &msg);

    // Obtaining the local unique ID
    uint8_t my_unique_id[sizeof(msg.unique_id.data)];
    getUniqueID(my_unique_id);

    // Matching the received UID against the local one
    if (memcmp(msg.unique_id.data, my_unique_id, msg.unique_id.len) != 0) {
        printf("Mismatching allocation response\n");
        DNA.node_id_allocation_unique_id_offset = 0;
        // No match, return
        return;
    }

    if (msg.unique_id.len < sizeof(msg.unique_id.data)) {
        // The allocator has confirmed part of unique ID, switching to
        // the next stage and updating the timeout.
        DNA.node_id_allocation_unique_id_offset = msg.unique_id.len;
        DNA.send_next_node_id_allocation_request_at_ms -= UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MIN_REQUEST_PERIOD_MS;

        printf("Matching allocation response: %d\n", msg.unique_id.len);
    } else {
        // Allocation complete - copying the allocated node ID from the message
        canardSetLocalNodeID(ins, msg.node_id);
        printf("Node ID allocated: %d\n", msg.node_id);
    }
}

/*
  ask for a dynamic node allocation
 */
static void request_DNA()
{
    const uint32_t now = millis32();
    static uint8_t node_id_allocation_transfer_id = 0;

    DNA.send_next_node_id_allocation_request_at_ms =
        now + UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MIN_REQUEST_PERIOD_MS + (random() % UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_MAX_FOLLOWUP_DELAY_MS);

    // Structure of the request is documented in the DSDL definition
    // See http://uavcan.org/Specification/6._Application_level_functions/#dynamic-node-id-allocation
    uint8_t allocation_request[CANARD_CAN_FRAME_MAX_DATA_LEN - 1];
    allocation_request[0] = (uint8_t)(PREFERRED_NODE_ID << 1U);

    if (DNA.node_id_allocation_unique_id_offset == 0) {
        allocation_request[0] |= 1;    // First part of unique ID
    }

    uint8_t my_unique_id[16];
    getUniqueID(my_unique_id);

    static const uint8_t MaxLenOfUniqueIDInRequest = 6;
    uint8_t uid_size = (uint8_t)(16 - DNA.node_id_allocation_unique_id_offset);

    if (uid_size > MaxLenOfUniqueIDInRequest) {
        uid_size = MaxLenOfUniqueIDInRequest;
    }

    memmove(&allocation_request[1], &my_unique_id[DNA.node_id_allocation_unique_id_offset], uid_size);

    // Broadcasting the request
    const int16_t bcast_res = canardBroadcast(&canard, UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_SIGNATURE, UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_ID,
                                              &node_id_allocation_transfer_id, CANARD_TRANSFER_PRIORITY_LOW, &allocation_request[0], (uint16_t)(uid_size + 1));
    if (bcast_res < 0) {
        printf("Could not broadcast ID allocation req; error %d\n", bcast_res);
    }

    // Preparing for timeout; if response is received, this value will be updated from the callback.
    DNA.node_id_allocation_unique_id_offset = 0;
}

/*
 This callback is invoked by the library when a new message or request or response is received.
*/
static void onTransferReceived(CanardInstance* ins, CanardRxTransfer* transfer)
{
    // switch on data type ID to pass to the right handler function
    if (transfer->transfer_type == CanardTransferTypeRequest) {
        // check if we want to handle a specific service request
        switch (transfer->data_type_id) {
        case UAVCAN_PROTOCOL_GETNODEINFO_ID: {
            handle_GetNodeInfo(ins, transfer);
            break;
        }
        case UAVCAN_PROTOCOL_PARAM_GETSET_ID: {
            handle_param_GetSet(ins, transfer);
            break;
        }
        case UAVCAN_PROTOCOL_PARAM_EXECUTEOPCODE_ID: {
            handle_param_ExecuteOpcode(ins, transfer);
            break;
        }
        case UAVCAN_PROTOCOL_RESTARTNODE_ID: {
            handle_RestartNode(ins, transfer);
            break;
        }
        }
    }
    if (transfer->transfer_type == CanardTransferTypeBroadcast) {
        // check if we want to handle a specific broadcast message
        switch (transfer->data_type_id) {
        case UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_ID: {
            handle_DNA_Allocation(ins, transfer);
            break;
        }
        }
    }
}

/*
 This callback is invoked by the library when it detects beginning of a new transfer on the bus that can be received
 by the local node.
 If the callback returns true, the library will receive the transfer.
 If the callback returns false, the library will ignore the transfer.
 All transfers that are addressed to other nodes are always ignored.

 This function must fill in the out_data_type_signature to be the signature of the message.
 */
static bool shouldAcceptTransfer(const CanardInstance* ins, uint64_t* out_data_type_signature, uint16_t data_type_id, CanardTransferType transfer_type, uint8_t source_node_id)
{
    (void)ins;
    (void)source_node_id;
    if (transfer_type == CanardTransferTypeRequest) {
        // check if we want to handle a specific service request
        switch (data_type_id) {
        case UAVCAN_PROTOCOL_GETNODEINFO_ID: {
            *out_data_type_signature = UAVCAN_PROTOCOL_GETNODEINFO_REQUEST_SIGNATURE;
            return true;
        }
        case UAVCAN_PROTOCOL_PARAM_GETSET_ID: {
            *out_data_type_signature = UAVCAN_PROTOCOL_PARAM_GETSET_SIGNATURE;
            return true;
        }
        case UAVCAN_PROTOCOL_PARAM_EXECUTEOPCODE_ID: {
            *out_data_type_signature = UAVCAN_PROTOCOL_PARAM_EXECUTEOPCODE_SIGNATURE;
            return true;
        }
        case UAVCAN_PROTOCOL_RESTARTNODE_ID: {
            *out_data_type_signature = UAVCAN_PROTOCOL_RESTARTNODE_SIGNATURE;
            return true;
        }
        }
    }
    if (transfer_type == CanardTransferTypeBroadcast) {
        // see if we want to handle a specific broadcast packet
        switch (data_type_id) {
        case UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_ID: {
            *out_data_type_signature = UAVCAN_PROTOCOL_DYNAMIC_NODE_ID_ALLOCATION_SIGNATURE;
            return true;
        }
        }
    }
    // we don't want any other messages
    return false;
}

/*
  send the 1Hz NodeStatus message. This is what allows a node to show
  up in the DroneCAN GUI tool and in the flight controller logs
 */
static void send_NodeStatus(void)
{
    uint8_t buffer[UAVCAN_PROTOCOL_NODESTATUS_MAX_SIZE];

    node_status.uptime_sec = micros64() / 1000000ULL;
    node_status.health = UAVCAN_PROTOCOL_NODESTATUS_HEALTH_OK;
    node_status.mode = UAVCAN_PROTOCOL_NODESTATUS_MODE_OPERATIONAL;
    node_status.sub_mode = 0;
    // put whatever you like in here for display in GUI
    node_status.vendor_specific_status_code = 1234;

    uint32_t len = uavcan_protocol_NodeStatus_encode(&node_status, buffer);

    // we need a static variable for the transfer ID. This is
    // incremeneted on each transfer, allowing for detection of packet
    // loss
    static uint8_t transfer_id;

    canardBroadcast(&canard, UAVCAN_PROTOCOL_NODESTATUS_SIGNATURE, UAVCAN_PROTOCOL_NODESTATUS_ID, &transfer_id, CANARD_TRANSFER_PRIORITY_LOW, buffer, len);
}

/*
  This function is called at 1 Hz rate from the main loop.
*/
void dronecan_node_cleanup_1000ms(void)
{
    uint64_t time_ms = HAL_GetTick();
    uint64_t time_us = time_ms * 1000;
    /*
      Purge transfers that are no longer transmitted. This can free up some memory
    */
    canardCleanupStaleTransfers(&canard, time_us);

    /*
      Transmit the node status message
    */
    send_NodeStatus();
}

/*
  send BatteryInfo at 10Hz
*/
void dronecan_send_battery_info_1000ms(void)
{
    struct uavcan_equipment_power_BatteryInfo pkt;
    memset(&pkt, 0, sizeof(pkt));
    uint8_t buffer[UAVCAN_EQUIPMENT_POWER_BATTERYINFO_MAX_SIZE];

    // make up some synthetic status data
    pkt.temperature = rte_dronecan_battery.temperature_K;
    pkt.voltage = rte_dronecan_battery.voltage;
    pkt.current = rte_dronecan_battery.current;

    /*
      Note!! fill in all remaining fields from the DSDL
     */

    pkt.battery_id = settings.battery_index;
    pkt.model_instance_id = 0;
    pkt.model_name.len = strlen(BATTERY_MANUFACTURER_NAME);
    strncpy((char*)pkt.model_name.data, BATTERY_MANUFACTURER_NAME, sizeof(pkt.model_name.data));
    pkt.state_of_charge_pct = rte_dronecan_battery.remaining_capacity;
    pkt.state_of_health_pct = 50;
    pkt.full_charge_capacity_wh = rte_dronecan_battery.total_capacity_Ah * 22.2;
    pkt.remaining_capacity_wh = rte_dronecan_battery.remaining_capacity / 100.0 * pkt.full_charge_capacity_wh;

    uint32_t len = uavcan_equipment_power_BatteryInfo_encode(&pkt, buffer);

    // we need a static variable for the transfer ID. This is
    // incremeneted on each transfer, allowing for detection of packet
    // loss
    static uint8_t transfer_id;

    canardBroadcast(&canard, UAVCAN_EQUIPMENT_POWER_BATTERYINFO_SIGNATURE, UAVCAN_EQUIPMENT_POWER_BATTERYINFO_ID, &transfer_id, CANARD_TRANSFER_PRIORITY_LOW, buffer, len);
}

static CAN_TxHeaderTypeDef conv_tx_frame(const CanardCANFrame* txf)
{
    CAN_TxHeaderTypeDef header = {0};
    if (txf->id < 0x800) {
        header.StdId = txf->id;
        header.IDE = CAN_ID_STD;
    } else {
        header.ExtId = txf->id;
        header.IDE = CAN_ID_EXT;
    }
    header.RTR = CAN_RTR_DATA;
    header.DLC = txf->data_len;

    return header;
}

static void conv_rx_frame(const CAN_RxHeaderTypeDef* rx_header_ptr, CanardCANFrame* out_frame_ptr)
{
    if (rx_header_ptr->IDE == CAN_ID_STD) out_frame_ptr->id = rx_header_ptr->StdId;
    else out_frame_ptr->id = rx_header_ptr->ExtId;
    out_frame_ptr->data_len = rx_header_ptr->DLC;
    out_frame_ptr->iface_id = 0;
}

/*
  Transmits all frames from the TX queue, receives up to one frame.
*/
static void processTxRxOnce(void)
{
    // Transmitting
    for (const CanardCANFrame* txf = NULL; (txf = canardPeekTxQueue(&canard)) != NULL;) {
        const int16_t tx_res = canardSTM32Transmit(txf);
        if (tx_res < 0) {    // Failure - drop the frame
            canardPopTxQueue(&canard);
        } else if (tx_res > 0)    // Success - just drop the frame
        {
            canardPopTxQueue(&canard);
        } else    // Timeout - just exit and try again later
        {
            break;
        }
    }

    CanardCANFrame rx_frame = {0};
    const uint64_t timestamp = micros64();
    const int16_t rx_res = canardSTM32Receive(&rx_frame);
    if (rx_res < 0) {
        UNREACHABLE();
    } else if (rx_res > 0)    // Success - process the frame
    {
        canardHandleRxFrame(&canard, &rx_frame, timestamp);
    }
}

void dronecan_init(void)
{
    /* Initializing the Libcanard instance. */
    canardInit(&canard, memory_pool, sizeof(memory_pool), onTransferReceived, shouldAcceptTransfer, NULL);
    if (MY_NODE_ID > 0) {
        canardSetLocalNodeID(&canard, MY_NODE_ID);
    }
}

void dronecan_process_tx_rx_1ms(void)
{
    processTxRxOnce();
    if (canardGetLocalNodeID(&canard) == CANARD_BROADCAST_NODE_ID) {
        // waiting for DNA
    }

    // see if we are still doing DNA
    if (canardGetLocalNodeID(&canard) == CANARD_BROADCAST_NODE_ID) {
        // we're still waiting for a DNA allocation of our node ID
        if (millis32() > DNA.send_next_node_id_allocation_request_at_ms) {
            request_DNA();
        }
        return;
    }
}
