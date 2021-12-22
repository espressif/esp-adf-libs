// Copyright 2021 Espressif Systems (Shanghai) CO., LTD
// All rights reserved.

#ifndef ESP_JPEG_DEC_H
#define ESP_JPEG_DEC_H

#ifdef __cplusplus
extern "C" {
#endif

#define DEFAULT_JPEG_DEC_CONFIG() {               \
    .output_type               = JPEG_RGB565,     \
}

#define JPEG_DEC_MAX_MARKER_CHECK_LEN (1024)

/* Error code */
typedef enum {
    JPEG_DEC_ERR_OK = 0,            /* Succeeded */
    JPEG_DEC_ERR_INP = -1,          /* Device error or wrong termination of input stream */
    JPEG_DEC_ERR_MEM = -2,          /* Insufficient memory pool for the image */
    JPEG_DEC_ERR_NO_MORE_DATA = -3, /* Input data is not enough */
    JPEG_DEC_ERR_PAR = -4,          /* Parameter error */
    JPEG_DEC_ERR_FMT1 = -5,         /* Data format error (may be damaged data) */
    JPEG_DEC_ERR_FMT2 = -6,         /* Right format but not supported */
    JPEG_DEC_ERR_FMT3 = -7          /* Not supported JPEG standard */
} jpeg_dec_error_t;

/* Jpeg dec outdata type */
typedef enum {
    JPEG_RGB565 = 0,
    JPEG_RGB888 = 1,
} jpeg_dec_out_type_t;

typedef void* jpeg_dec_handle_t;

/* Jpeg dec user need to config */
typedef struct {
    jpeg_dec_out_type_t output_type; /*!< jpeg_dec_out_type 1:rgb888 0:rgb565 */
} jpeg_dec_config_t;

/* Jpeg dec out info */
typedef struct {
    int width;                      /* Number of pixels in the horizontal direction */
    int height;                     /* Number of pixels in the vertical direction */
    int component_num;              /* Number of color component*/
    uint8_t component_id[3];        /* ID of color component*/
    uint8_t x_factory[3];           /* Size factory in the x direction*/
    uint8_t y_factory[3];           /* Size factory in the y direction*/
    uint16_t nrst;                  /* Restart inverval */
    uint8_t huffbits[2][2][16];     /* Huffman bit distribution tables [id][dcac] */
    uint16_t huffdata[2][2][256];   /* Huffman decoded data tables [id][dcac] */
    uint8_t qtid[3];                /* Quantization table ID of each component */
    int16_t qt_tbl[4][64];         /* Dequantizer tables [id] */
} jpeg_dec_header_info_t;

/* Jpeg dec io control */
typedef struct {
    unsigned char *inbuf;
    int inbuf_len;
    int inbuf_remain;
    unsigned char *outbuf;
} jpeg_dec_io_t;

/**
 * @brief      Create a Jpeg decode handle, set user config info to decode handle
 * 
 * @param[in]      config        The configuration information
 * 
 * @return     other values: The JPEG decoder handle
 *             NULL: failed  
 */
jpeg_dec_handle_t *jpeg_dec_open(jpeg_dec_config_t *config);

/**
 * @brief      Parse picture data header, and out put info to user
 *
 * @param[in]      jpeg_dec        jpeg decoder handle
 * 
 * @param[in]      io              struct of jpeg_dec_io_t
 * 
 * @param[out]     out_info        output info struct to user 
 * 
 * @return     jpeg_dec_error_t
 *             - JPEG_DEC_ERR_OK: on success
 *             - Others: error occurs    
 */
jpeg_dec_error_t jpeg_dec_parse_header(jpeg_dec_handle_t *jpeg_dec, jpeg_dec_io_t *io, jpeg_dec_header_info_t *out_info);

/**
 * @brief      Decode one Jpeg picture 
 *
 * @param[in]      jpeg_dec    jpeg decoder handle
 * 
 * @param[in]      io          struct of jpeg_dec_io_t 
 * 
 * @return     jpeg_dec_error_t
 *             - JPEG_DEC_ERR_OK: on success
 *             - Others: error occurs    
 */
jpeg_dec_error_t jpeg_dec_process(jpeg_dec_handle_t *jpeg_dec, jpeg_dec_io_t *io);

/**
 * @brief      Deinitialize Jpeg decode handle 
 * 
 * @param[in]      jpeg_dec    jpeg decoder handle     
 * 
 * @return     jpeg_dec_error_t
 *             - JPEG_DEC_ERR_OK: on success
 *             - Others: error occurs    
 */
jpeg_dec_error_t jpeg_dec_close(jpeg_dec_handle_t *jpeg_dec);

/**
 * Example usage:
 * @code{c}
 * 
 * // Function for decode one jpeg picture
 * // input_buf   input picture data
 * // len         input picture data length
 * int esp_jpeg_decoder_one_picture(unsigned char *input_buf, int len, unsigned char **output_buf)
 * {
 *      // Generate default configuration
 *      jpeg_dec_config_t config = DEFAULT_JPEG_DEC_CONFIG();
 *      
 *      // Empty handle to jpeg_decoder
 *      jpeg_dec_handle_t jpeg_dec = NULL;
 *      
 *      // Create jpeg_dec
 *      jpeg_dec = jpeg_dec_open(&config);
 * 
 *      // Create io_callback handle
 *      jpeg_dec_io_t *jpeg_io = calloc(1, sizeof(jpeg_dec_io_t));
 *      if (jpeg_io == NULL) {
 *          return ESP_FAIL;
 *      }
 *      
 *      // Create out_info handle
 *      jpeg_dec_header_info_t *out_info = calloc(1, sizeof(jpeg_dec_header_info_t));
 *      if (out_info == NULL) {
 *          return ESP_FAIL;
 *      }
 * 
 *      // Set input buffer and buffer len to io_callback
 *      jpeg_io->inbuf = input_buf;
 *      jpeg_io->inbuf_len = len;
 *      
 *      int ret = 0;
 *      // Parse jpeg picture header and get picture for user and decoder
 *      ret = jpeg_dec_parse_header(jpeg_dec, jpeg_io, out_info);
 *      if (ret < 0) {
 *          return ret;
 *      }
 * 
 *      // Calloc out_put data buffer and update inbuf ptr and inbuf_len
 *      int outbuf_len;
 *      if (config.output_type == JPEG_RGB565) {
 *          outbuf_len = out_info->height * out_info->height * 2;
 *      } else if (config.output_type == JPEG_RGB888) {
 *          outbuf_len = out_info->height * out_info->height * 3;
 *      } else {
 *          return ESP_FAIL;
 *      }
 *      unsigned char *out_buf = calloc(1, outbuf_len);
 *      jpeg_io->outbuf = out_buf;
 *      *output_buf = out_buf;
 *      int inbuf_consumed = jpeg_io->inbuf_len - jpeg_io->inbuf_remain;
 *      jpeg_io->inbuf = input_buf + inbuf_consumed;
 *      jpeg_io->inbuf_len = jpeg_io->inbuf_remain;
 * 
 *      // Start decode jpeg raw data
 *      ret = jpeg_dec_process(jpeg_dec, jpeg_io);
 *      if (ret < 0) {
 *          return ret;
 *      }
 * 
 *      // Decoder deinitialize 
 *      jpeg_dec_close(jpeg_dec);
 *      free(out_info);
 *      free(jpeg_io);
 *      return ESP_OK;
 * }
 * 
 * @endcode
 *  
 */

#ifdef __cplusplus
}
#endif

#endif
