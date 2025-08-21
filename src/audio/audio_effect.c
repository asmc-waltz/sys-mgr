/**
 * @file audio_effect.c
 *
 * WAV parser (chunk-based RIFF) on mmap'ed file + playback via audio_mgr.
 * - full RIFF chunk scan for "fmt " and "data"
 * - supports PCM and WAVEFORMATEXTENSIBLE (treated as PCM if subtype indicates PCM)
 * - no fread: file is mmap()ed entirely and parsed via pointer arithmetic
 * - playback feeds audio_mgr_write_mmap in chunks
 */

/*********************
 *      INCLUDES
 *********************/
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <errno.h>

#include <audio.h>

#include <log.h>

/*********************
 *      DEFINES
 *********************/

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  GLOBAL VARIABLES
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
/* read little-endian helpers from byte pointer */
static inline uint16_t rd_u16le(const uint8_t *p) { return (uint16_t)(p[0] | (p[1] << 8)); }
static inline uint32_t rd_u32le(const uint8_t *p) { return (uint32_t)(p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24)); }

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

/**********************
 *   STATIC FUNCTIONS
 **********************/
/* parse "fmt " chunk payload (pointer to payload, size) into audio_format */
static int parse_fmt_chunk(const uint8_t *p, uint32_t size, struct audio_format *out)
{
    if (size < 16) return AUDIO_E_INVAL;

    uint16_t audio_format = rd_u16le(p + 0);
    uint16_t channels     = rd_u16le(p + 2);
    uint32_t sample_rate  = rd_u32le(p + 4);
    /* uint32_t byte_rate = rd_u32le(p + 8); */
    uint16_t block_align  = rd_u16le(p + 12);
    uint16_t bits_per_sample = rd_u16le(p + 14);

    p += 16;

    if (size > 16) {
        /* there is an extension size */
        uint16_t cb = 0;
        if ((size - 16) >= 2) {
            cb = rd_u16le(p);
            p += 2;
        } else {
            return AUDIO_E_INVAL;
        }

        /* WAVEFORMATEXTENSIBLE handling */
        if (audio_format == 0xFFFE) { /* extensible */
            /* need at least 22 bytes for extension: valid_bits, channel_mask, subformat[16] */
            if (cb < 22 || (size < 16 + 2 + 22)) return AUDIO_E_INVAL;
            /* For brevity we accept extensible as PCM (you may check GUID if needed) */
            /* skip extension content */
            /* p currently points to extension start */
            /* We don't parse GUID here; assume PCM subtype */
            /* skip 22 bytes */
            /* (valid_bits, channel_mask, subformat[16]) */
            /* no further action required for our use-case */
        }
        /* otherwise, non-extensible additional bytes are ignored/skipped */
    }

    /* accept only PCM */
    if (audio_format != 0x0001 && audio_format != 0xFFFE) return AUDIO_E_NOSUP;

    out->pcm_format = audio_bits_to_sndfmt(bits_per_sample);
    if (out->pcm_format == SND_PCM_FORMAT_UNKNOWN) return AUDIO_E_NOSUP;

    out->channels = channels;
    out->sample_rate = sample_rate;
    out->bits_per_sample = bits_per_sample;
    out->block_align = block_align;

    return AUDIO_OK;
}

/* parse mmap'ed WAV file: find fmt + data chunk */
static int wav_parse_mmap(const uint8_t *base, size_t size, struct wav_map *wm)
{
    /* minimal RIFF header size = 12 bytes (RIFF + size + WAVE) */
    if (size < 12) return AUDIO_E_INVAL;
    if (memcmp(base, "RIFF", 4) != 0) return AUDIO_E_INVAL;
    if (memcmp(base + 8, "WAVE", 4) != 0) return AUDIO_E_INVAL;

    const uint8_t *p = base + 12;
    const uint8_t *end = base + size;
    int have_fmt = 0;
    int have_data = 0;

    while (p + 8 <= end) {
        const uint8_t *chunk_id = p;
        uint32_t chunk_size = rd_u32le(p + 4);
        const uint8_t *payload = p + 8;
        const uint8_t *next = payload + chunk_size + (chunk_size & 1); /* pad */

        if (payload + chunk_size > end) return AUDIO_E_INVAL;

        if (memcmp(chunk_id, "fmt ", 4) == 0) {
            int rc = parse_fmt_chunk(payload, chunk_size, &wm->fmt);
            if (rc < 0) return rc;
            have_fmt = 1;
        } else if (memcmp(chunk_id, "data", 4) == 0) {
            wm->data = payload;
            wm->data_size = chunk_size;
            have_data = 1;
        } else {
            /* ignore unknown chunk */
        }

        p = next;
    }

    return (have_fmt && have_data) ? AUDIO_OK : AUDIO_E_INVAL;
}

/**********************
 *   GLOBAL FUNCTIONS
 **********************//* map bits -> snd pcm format */
snd_pcm_format_t audio_bits_to_sndfmt(unsigned bits)
{
    switch (bits) {
    case 8:  return SND_PCM_FORMAT_U8;
    case 16: return SND_PCM_FORMAT_S16_LE;
    case 24: return SND_PCM_FORMAT_S24_LE;
    case 32: return SND_PCM_FORMAT_S32_LE;
    default: return SND_PCM_FORMAT_UNKNOWN;
    }
}

/* public: open+map file and parse */
int wav_map_open(const char *path, struct wav_map *out)
{
    int fd;
    struct stat st;
    void *base;
    int rc;

    if (!path || !out) return AUDIO_E_INVAL;
    memset(out, 0, sizeof(*out));
    out->fd = -1;

    fd = open(path, O_RDONLY);
    if (fd < 0) {
        LOG_ERROR("open('%s'): %s", path, strerror(errno));
        return AUDIO_E_IO;
    }

    if (fstat(fd, &st) < 0) {
        LOG_ERROR("fstat('%s'): %s", path, strerror(errno));
        close(fd);
        return AUDIO_E_IO;
    }
    if ((size_t)st.st_size < 12) {
        LOG_ERROR("file too small: %s", path);
        close(fd);
        return AUDIO_E_INVAL;
    }

    base = mmap(NULL, (size_t)st.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (base == MAP_FAILED) {
        LOG_ERROR("mmap('%s'): %s", path, strerror(errno));
        close(fd);
        return AUDIO_E_IO;
    }

    out->fd = fd;
    out->base = base;
    out->size = (size_t)st.st_size;

    rc = wav_parse_mmap((const uint8_t *)base, out->size, out);
    if (rc < 0) {
        LOG_ERROR("wav parse failed for '%s'", path);
        munmap(base, out->size);
        close(fd);
        memset(out, 0, sizeof(*out));
        return rc;
    }

    LOG_DEBUG("wav_map_open: '%s' ch=%u sr=%u bps=%u data=%zu",
          path, out->fmt.channels, out->fmt.sample_rate,
          out->fmt.bits_per_sample, out->data_size);

    return AUDIO_OK;
}

/* close mapping */
void wav_map_close(struct wav_map *wm)
{
    if (!wm) return;
    if (wm->base && wm->size) munmap(wm->base, wm->size);
    if (wm->fd >= 0) close(wm->fd);
    memset(wm, 0, sizeof(*wm));
}

/* play from mapping (handles format mismatch options) */
int audio_play_wav_map(struct audio_mgr *mgr, const struct wav_map *wm)
{
    int rc;

    if (!mgr || !wm) return AUDIO_E_INVAL;

    /* if skip_format_check is set, we trust caller and play without matching */
    if (!mgr->skip_format_check) {
        /* if formats match, proceed */
        if (wm->fmt.channels == mgr->fmt.channels &&
            wm->fmt.sample_rate == mgr->fmt.sample_rate &&
            wm->fmt.pcm_format == mgr->fmt.pcm_format) {
            /* formats match */
        } else {
            /* mismatch */
            if (mgr->auto_reinit) {
                LOG_INFO("format mismatch detected; auto_reinit enabled -> reinit device");
                rc = audio_mgr_reinit(mgr, wm->fmt.pcm_format, wm->fmt.channels, wm->fmt.sample_rate);
                if (rc < 0) {
                    LOG_ERROR("audio_mgr_reinit failed");
                    return rc;
                }
            } else {
                LOG_ERROR("format mismatch (file vs device) and auto_reinit disabled");
                return AUDIO_E_INVAL;
            }
        }
    } else {
        LOG_DEBUG("skip_format_check enabled: playing without checking format");
    }

    /* compute total frames in the data chunk */
    size_t frame_size = mgr->frame_size;
    if (frame_size == 0) return AUDIO_E_INVAL;
    size_t total_frames = wm->data_size / frame_size;

    /* stream in reasonable chunks */
    const size_t chunk_frames = 4096;
    size_t done = 0;

    while (done < total_frames) {
        size_t to_do = total_frames - done;
        if (to_do > chunk_frames) to_do = chunk_frames;

        LOG_DEBUG("pcm state=%s - loaded %d - total %d", \
                  snd_pcm_state_name(snd_pcm_state(mgr->pcm)), done, total_frames);

        rc = audio_mgr_write_mmap(mgr, wm->data + done * frame_size, to_do);
        if (rc < 0) {
            LOG_ERROR("audio_mgr_write_mmap failed");
            return rc;
        }
        // TODO: reverity
        snd_pcm_start(mgr->pcm);
        done += to_do;
    }

    return AUDIO_OK;
}

/* convenience: open->play->close */
int audio_play_wav_file(struct audio_mgr *mgr, const char *path)
{
    struct wav_map wm;
    int rc;

    rc = wav_map_open(path, &wm);
    if (rc < 0) return rc;

    rc = audio_play_wav_map(mgr, &wm);
    wav_map_close(&wm);
    return rc;
}
