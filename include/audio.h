/**
 * @file audio.h
 *
 * Public header for audio manager + wav effect.
 * - RIFF/WAVE chunk-based parser
 * - mmap() the WAV file (zero-copy file -> memory)
 * - ALSA mmap interleaved playback (zero-copy into kernel ring)
 * - per-channel and master software gain
 * - option: auto reinit ALSA when next file has different format
 * - option: skip format check to optimize when caller guarantees formats match
 */

#ifndef G_AUDIO_H
#define G_AUDIO_H
/*********************
 *      INCLUDES
 *********************/
#include <stddef.h>
#include <stdint.h>
#include <alsa/asoundlib.h>

#include <log.h>

/*********************
 *      DEFINES
 *********************/
/* result codes */
#define AUDIO_OK        0
#define AUDIO_ERR      -1
#define AUDIO_E_INVAL  -2
#define AUDIO_E_NOSUP  -3
#define AUDIO_E_IO     -4

/* maximum channels supported by software gains */
#define AUDIO_MAX_CHANNELS 8

/* audio format description */
struct audio_format {
    snd_pcm_format_t pcm_format;   /* ALSA format enum */
    uint32_t sample_rate;      /* sample rate Hz */
    uint32_t channels;         /* channels count */
    uint32_t bits_per_sample;  /* bits per sample */
    uint32_t block_align;      /* bytes per frame (channels * bits/8) */
};

/* audio manager context */
struct audio_mgr {
    /* ALSA handle */
    snd_pcm_t *pcm;
    /* device name (heap allocated) */
    char *device_name;
    /* current hw format */
    struct audio_format fmt;
    /* bytes per frame */
    size_t frame_size;

    /* software gain */
    float master_gain;             /* 0.0 .. 1.0 */
    float ch_gain[AUDIO_MAX_CHANNELS];

    /* behavior options */
    int auto_reinit;               /* if true, reinit hw when new file format differs */
    int skip_format_check;         /* if true, do not check file format when playing */

    /* reserved for future fields */
    void *priv;
};

/* mapping of WAV file via mmap */
struct wav_map {
    int fd;                /* open file descriptor */
    void *base;            /* mmap base pointer (whole file) */
    size_t size;           /* mapped file size */
    const uint8_t *data;   /* pointer to PCM data within map */
    size_t data_size;      /* size of PCM data */
    struct audio_format fmt; /* file format */
};

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  GLOBAL VARIABLES
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/

/**********************
 *  GLOBAL PROTOTYPES
 **********************/
/* utility: convert bits -> ALSA format */
snd_pcm_format_t audio_bits_to_sndfmt(unsigned bits);

/* ---------------- manager API ------------------ */

/* Initialize manager with requested device and format.
 * On success, mgr is populated and ready for play.
 * caller must provide a valid 'mgr' pointer.
 */
int audio_mgr_init(struct audio_mgr *mgr, \
           const char *device_name, \
           snd_pcm_format_t pcm_format, \
           uint32_t channels, \
           uint32_t sample_rate, \
           uint32_t buffer_time_us, \
           uint32_t period_time_us);

/* Reinitialize current manager with new format (close + init).
 * Useful when auto_reinit is enabled.
 */
int audio_mgr_reinit(struct audio_mgr *mgr, \
             snd_pcm_format_t pcm_format, \
             uint32_t channels, \
             uint32_t sample_rate);

/* Prepare (snd_pcm_prepare) - safe to call when recovering from xruns */
int audio_mgr_prepare(struct audio_mgr *mgr);

/* Release all resources (snd_pcm_drain + close + free device name) */
void audio_mgr_release(struct audio_mgr *mgr);

/* Options */
int audio_mgr_set_auto_reinit(struct audio_mgr *mgr, int enable); /* 0/1 */
int audio_mgr_set_skip_format_check(struct audio_mgr *mgr, int enable); /* 0/1 */

/* Gain control (clamped 0.0..1.0). Return AUDIO_OK or error. */
int audio_mgr_set_master_gain(struct audio_mgr *mgr, float gain);
int audio_mgr_set_channel_gain(struct audio_mgr *mgr, uint32_t ch, float gain);
int audio_mgr_set_channel_gains(struct audio_mgr *mgr, const float *gains, uint32_t n);

/* Zero-copy write via ALSA mmap: copy 'frames' frames from src into ALSA ring.
 * src layout = interleaved PCM matching mgr->fmt.
 */
int audio_mgr_write_mmap(struct audio_mgr *mgr, const void *src, size_t frames);

/* ---------------- wav mapper / parser API ------------------ */

/* Open WAV file, mmap entire file, parse RIFF chunk-by-chunk, locate fmt + data.
 * On success the struct wav_map is filled and file remains mmaped until wav_map_close.
 * Caller must call wav_map_close() when done.
 */
int wav_map_open(const char *path, struct wav_map *out);

/* Unmap file and close fd */
void wav_map_close(struct wav_map *wm);

/* ---------------- playback helpers ------------------ */

/* Play from an already-opened wav_map (uses audio_mgr_write_mmap).
 * Behavior when format mismatch:
 *  - if mgr->skip_format_check == 1: play anyway (caller guarantees match)
 *  - else if mgr->auto_reinit == 1: call audio_mgr_reinit() to match file format and play
 *  - else: return AUDIO_E_INVAL
 */
int audio_play_wav_map(struct audio_mgr *mgr, const struct wav_map *wm);

/* Convenience: open->play->close */
int audio_play_wav_file(struct audio_mgr *mgr, const char *path);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

/**********************
 *   STATIC FUNCTIONS
 **********************/

#endif /* G_AUDIO_H */
