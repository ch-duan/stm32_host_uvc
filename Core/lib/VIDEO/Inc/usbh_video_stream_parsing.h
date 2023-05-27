#ifndef _USBH_VIDEO_STREAM_PARSING_H
#define _USBH_VIDEO_STREAM_PARSING_H

#include "usbh_video.h"
#ifdef __cplusplus
extern "C" {
#endif


#define UVC_HEADER_SIZE_POS      0
#define UVC_HEADER_BIT_FIELD_POS 1

#define UVC_HEADER_FID_BIT       (1 << 0)
#define UVC_HEADER_EOF_BIT       (1 << 1)
#define UVC_HEADER_ERR_BIT       (1 << 6)

#define UVC_HEADER_SIZE          12


int video_stream_process_packet(uint16_t size);
void video_stream_init_buffers(uint8_t *buffer0, uint8_t *buffer1);
void video_stream_ready_update(void);
typedef void(*videoPacketArrived)(uint8_t* data,uint32_t len);

void videoPacketArrivedCallback(videoPacketArrived callback);
#ifdef __cplusplus
}
#endif

#endif
