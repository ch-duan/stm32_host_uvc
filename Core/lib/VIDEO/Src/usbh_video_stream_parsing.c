
#include "usbh_video_stream_parsing.h"

#include <stdbool.h>

#include "usbh_video.h"
#include "usbh_video_desc_parsing.h"


uint8_t uvc_prev_fid_state = 0;

uint32_t uvc_curr_frame_length = 0;

// This value should be used by external software

// Flags

uint8_t uvc_parsing_initialized = false;

bool uvc_frame_start_detected = true;

// Previous packet was EOF
bool uvc_prev_packet_eof = true;

extern volatile uint8_t tmp_packet_framebuffer[UVC_RX_FIFO_SIZE_LIMIT];

// Pointers to a framebuffers to store captured frame

videoPacketArrived videoCallback = NULL;

uint8_t* uvc_framebuffer0_ptr = NULL;
uint8_t* uvc_framebuffer1_ptr = NULL;

// Pointer to a buffer that is FILLING now
uint8_t* uvc_curr_framebuffer_ptr = NULL;

extern USBH_VIDEO_TargetFormat_t USBH_VIDEO_Target_Format;

//****************************************************************************

void video_stream_add_packet_data(uint8_t* buf, uint16_t data_size);
uint8_t video_stream_switch_buffers(void);

void videoPacketArrivedCallback(videoPacketArrived callback) {
  videoCallback = callback;
}

//****************************************************************************
// size - new packet size
int video_stream_process_packet(uint16_t size) {
  printf("size:%d\r\n", size);
  for (uint32_t i = 0; i < size; i++) {
    if (i % 1024 == 0) {
      printf("\r\n");
    }
    printf("%02X ", tmp_packet_framebuffer[i]);
  }
  printf("\r\n");

  if ((size < 2) && (size > UVC_RX_FIFO_SIZE_LIMIT))
    return 0;  // error

  if (!uvc_parsing_initialized) {
    video_stream_switch_buffers();  // try to switch buffers
  }

  if (size <= UVC_HEADER_SIZE) {
  } else if (size > UVC_HEADER_SIZE) {
    // Get FID bit state
    if (tmp_packet_framebuffer[UVC_HEADER_BIT_FIELD_POS] & UVC_HEADER_ERR_BIT) {
    USBH_UsrLog("uvc error bit is set\r\n");
    }
    uint8_t masked_fid = (tmp_packet_framebuffer[UVC_HEADER_BIT_FIELD_POS] & UVC_HEADER_FID_BIT);
    if ((masked_fid != uvc_prev_fid_state) && (uvc_prev_packet_eof == true)) {
      // Detected FIRST packet of the frame
      USBH_UsrLog("find a new frame\r\n");
      uvc_curr_frame_length = 0;
      uvc_frame_start_detected = true;
    }
    uvc_prev_fid_state = masked_fid;

    uint16_t data_size = size - UVC_HEADER_SIZE;
    video_stream_add_packet_data((uint8_t*) (tmp_packet_framebuffer + UVC_HEADER_SIZE), data_size);
    uvc_curr_frame_length += data_size;

    if (tmp_packet_framebuffer[UVC_HEADER_BIT_FIELD_POS] & UVC_HEADER_EOF_BIT)  // Last packet in frame
    {
      uvc_prev_packet_eof = true;
      if (uvc_frame_start_detected == false) {
        USBH_UsrLog("find a bad frame\r\n");
        uvc_curr_frame_length = 0;
        return -1;  // Bad frame data
      }

      if (USBH_VIDEO_Target_Format == USBH_VIDEO_MJPEG) {
        USBH_UsrLog("frame size:%d", uvc_curr_frame_length);
        // for (uint32_t i = 0; i < uvc_curr_frame_length; i++) {
        //   if (i % 1024 == 0) {
        //     printf("\r\n");
        //   }
        //   printf("%02X ", uvc_framebuffer1_ptr[i]);
        // }
        // printf("\r\n");
        // call frame arrived
        if (videoCallback != NULL) {
          videoCallback(uvc_curr_framebuffer_ptr, uvc_curr_frame_length);
        }
        video_stream_switch_buffers();
        return 1;
      }
    } else {
      uvc_prev_packet_eof = false;
    }

    // if ((USBH_VIDEO_Target_Format == USBH_VIDEO_YUY2) && (uvc_curr_frame_length >= UVC_UNCOMP_FRAME_SIZE)) {
    //   if (uvc_frame_start_detected == 0)
    //     return -1;  // Bad frame data

    //   video_stream_switch_buffers();
    //   USBH_UsrLog("UVC_UNCOMP_FRAME:");
    //   //        for (uint32_t i = 0; i < uvc_curr_frame_length; i++) {
    //   //          if (i % 1024 == 0) {
    //   //            printf("\r\n");
    //   //          }
    //   //          printf("%02X ", uvc_framebuffer1_ptr[i]);
    //   //        }
    // }
    return 0;
  }
  return 0;
}

// Must be called when full fame is captured
uint8_t video_stream_switch_buffers(void) {
  if (uvc_curr_framebuffer_ptr == uvc_framebuffer0_ptr)
    uvc_curr_framebuffer_ptr = uvc_framebuffer1_ptr;
  else
    uvc_curr_framebuffer_ptr = uvc_framebuffer0_ptr;

  uvc_frame_start_detected = false;
  uvc_curr_frame_length = 0;
  return 1;
}

// Add data from received packet to the image framebuffer
// buf - pointer to the data source
void video_stream_add_packet_data(uint8_t* buf, uint16_t data_size) {
  if ((uvc_curr_frame_length + data_size) > UVC_UNCOMP_FRAME_SIZE) {
    uvc_curr_frame_length = UVC_UNCOMP_FRAME_SIZE;
    return;
  }
  // Copy data to a current framebuffer
  memcpy((void*) (uvc_curr_framebuffer_ptr + uvc_curr_frame_length), buf, data_size);
  for (int i = 0; i < data_size; i++) {
    if (*(uvc_curr_framebuffer_ptr + uvc_curr_frame_length + i) != buf[i]) {
      printf("error %d,%02X,%02X\r\n", i, *(uvc_curr_framebuffer_ptr + uvc_curr_frame_length + i), *(buf + i));
    }
  }
  uvc_curr_frame_length += data_size;
}

void video_stream_init_buffers(uint8_t* buffer0, uint8_t* buffer1) {
  if ((buffer0 == NULL) || (buffer1 == NULL))
    return;

  uvc_framebuffer0_ptr = buffer0;
  uvc_framebuffer1_ptr = buffer1;
  uvc_curr_framebuffer_ptr = uvc_framebuffer0_ptr;
  uvc_parsing_initialized = true;
}
