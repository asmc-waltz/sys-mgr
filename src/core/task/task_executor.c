#include <stdlib.h>
#include <stdint.h>

#include <dbus_comm.h>

#include <workqueue.h>
#include <task.h>
#include <log.h>


remote_cmd_t *create_remote_cmd(void)
{
    remote_cmd_t *cmd;

    cmd = calloc(1, sizeof(*cmd));
    if (!cmd) {
        return NULL;
    }

    return cmd;
}

void delete_remote_cmd(remote_cmd_t *cmd)
{
    if (!cmd) {
        LOG_WARN("Unable to delete cmd: null pointer");
        return;
    }

    free(cmd);
}

local_cmd_t *create_local_cmd()
{
    local_cmd_t *cmd = NULL;

    cmd = calloc(1, sizeof(*cmd));
    if (!cmd) {
        return NULL;
    }

    return cmd;
}


void delete_local_cmd(local_cmd_t *cmd)
{
    if (!cmd) {
        LOG_WARN("Unable to delete cmd: null pointer");
        return;
    }

    free(cmd);
}

int process_opcode_endless(uint32_t opcode, void *data)
{
    int rc = 0;

    switch (opcode) {
    case OP_ID_START_DBUS:
        rc = dbus_fn_thread_handler();
        break;
    // case OP_ID_START_IMU:
    //     rc = imu_kalman_init("/sys/bus/iio/devices/iio:device0/", 100, 0.001f, 0.003f, 0.03f);
    //     imu_kalman_set_debug(1);
    //     if (!rc)
    //         rc = imu_fn_thread_handler();
    //     break;
    default:
        LOG_ERROR("Opcode [%d] is invalid");
        break;
    }

    return rc;
}

int process_opcode(uint32_t opcode, void *data)
{
    int rc = 0;

    switch (opcode) {
    case OP_ID_LEFT_VIBRATOR:
    //     rc = rumble_trigger(2, 80, 150);
    //     break;
    // case OP_ID_RIGHT_VIBRATOR:
    //     rc = rumble_trigger(3, 80, 150);
    //     break;
    // case OP_ID_STOP_IMU:
    //     imu_fn_thread_stop();
    //     break;
    // case OP_ID_READ_IMU:
    //     struct imu_angles a = imu_get_angles();
    //     LOG_DEBUG("roll=%.2f pitch=%.2f yaw=%.2f\n", a.roll, a.pitch, a.yaw);
    //     break;
    case OP_ID_AUDIO_INIT:
        snd_sys_init();
        break;
    case OP_ID_AUDIO_RELEASE:
        snd_sys_release();
        break;
    case OP_ID_SOUND_PLAY:
        // TODO: support sound file path
        audio_play_sound("/usr/share/sounds/sound-icons/percussion-10.wav");
        break;

    default:
        LOG_ERROR("Opcode [%d] is invalid");
        break;
    }

    return rc;
}

int create_local_simple_task(uint8_t flow, uint8_t duration, uint32_t opcode)
{
    work_t *work = create_work(LOCAL, flow, duration, opcode, NULL);
    if (!work) {
        LOG_ERROR("Failed to create work from cmd");
        return EXIT_FAILURE;
    }

    push_work(work);

    return EXIT_SUCCESS;
}
