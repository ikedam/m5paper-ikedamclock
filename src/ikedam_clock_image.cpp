#include "ikedam_clock.hpp"
#include <M5EPD_Canvas.h>
#include <utility/pngle.h>
#include <rom/tjpgd.h>

namespace {
    struct FitInfo {
        float scale;
        uint32_t offsetX;
        uint32_t offsetY;
    };

    FitInfo fit(uint32_t destW, uint32_t destH, uint32_t srcW, uint32_t srcH) {
        FitInfo fitInfo;
        float xscale = static_cast<float>(destW) / srcW;
        float yscale = static_cast<float>(destH) / srcH;
        fitInfo.scale = (xscale < yscale) ? xscale : yscale;

        uint32_t fitWidth = ceil(srcW * fitInfo.scale);
        uint32_t fitHeight = ceil(srcH * fitInfo.scale);

        fitInfo.offsetX = (fitWidth < destW) ? (destW - fitWidth) / 2 : 0;
        fitInfo.offsetY = (fitHeight < destH) ? (destH - fitHeight) / 2 : 0;

        return fitInfo;
    };

    // convert 0-255 rgba -> 15-0 gray
    uint8_t rgb_to_gray(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
        // ITU-R Rec BT.601
        // 0 - 255 gray
        float gray256 = 0.299f * r + 0.587f * g + 0.114f * b;
        if (a < 255) {
            gray256 *= (a / 255.0f);
        }
        // 255 -> 0
        // 0 -> 15
        uint8_t gray16 = 15 - floor(gray256 / 16.0f);
        return gray16;
    }
}

namespace {
    struct pngle_data_t {
        M5EPD_Canvas* pImage;
        FitInfo fitInfo;
    };

    void pngle_init_callback(pngle_t *pngle, uint32_t w, uint32_t h) {
        pngle_data_t* pData = static_cast<pngle_data_t*>(pngle_get_user_data(pngle));
        pData->fitInfo = fit(
            pData->pImage->width(),
            pData->pImage->height(),
            w,
            h
        );
    }

    void pngle_draw_callback(pngle_t *pngle, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint8_t rgba[4]) {
        pngle_data_t* pData = static_cast<pngle_data_t*>(pngle_get_user_data(pngle));

        x = (uint32_t)ceil(x * pData->fitInfo.scale) + pData->fitInfo.offsetX;
        y = (uint32_t)ceil(y * pData->fitInfo.scale) + pData->fitInfo.offsetY;
        w = (uint32_t)ceil(w * pData->fitInfo.scale);
        h = (uint32_t)ceil(h * pData->fitInfo.scale);

        pData->pImage->fillRect(
            x,
            y,
            w,
            h,
            rgb_to_gray(rgba[0], rgba[1], rgba[2], rgba[3])
        );
    }

    void pngle_done_callback(pngle_t *pngle) {
        // pngle_data_t* pData = static_cast<pngle_data_t*>(pngle_get_user_data(pngle));
        // nothing to do.
    }
}

bool IkedamClock::drawPng(M5EPD_Canvas* pImage, Stream* pStream) {
    pngle_t* pngle = pngle_new();
    pngle_data_t data = {
        .pImage = pImage,
    };
    pngle_set_user_data(pngle, &data);
    pngle_set_init_callback(pngle, pngle_init_callback);
    pngle_set_draw_callback(pngle, pngle_draw_callback);
    pngle_set_done_callback(pngle, pngle_done_callback);

    // https://github.com/kikuchan/pngle/blob/master/README.md
    // Feed data to pngle
    uint8_t buf[1024];
    int remain = 0;
    size_t len;
    while ((len = pStream->readBytes(buf + remain, sizeof(buf) - remain)) > 0) {
        int fed = pngle_feed(pngle, buf, remain + len);
        if (fed < 0) {
            log_e("Error in pngle: %s", pngle_error(pngle));
            pngle_destroy(pngle);
            return false;
        }

        remain = remain + len - fed;
        if (remain > 0) {
            memmove(buf, buf + fed, remain);
        }
    }
    pngle_destroy(pngle);
    return true;
}

namespace {
    struct jd_data_t {
        M5EPD_Canvas* pImage;
        Stream* pStream;
        FitInfo fitInfo;
    };

    size_t jd_in_func (    /* Returns number of bytes read (zero on error) */
        JDEC* jd,       /* Decompression object */
        uint8_t* buff,  /* Pointer to the read buffer (null to remove data) */
        size_t nbyte    /* Number of bytes to read/remove */
    ) {
        jd_data_t* pData = static_cast<jd_data_t*>(jd->device);

        if (!buff) {
            if (nbyte > 1024) {
                nbyte = 1024;
            }
            char* buf = new char[nbyte];
            size_t read = pData->pStream->readBytes(buf, nbyte);
            delete[] buf;
            return read;
        }
        return pData->pStream->readBytes(buff, nbyte);
    }

    UINT jd_out_func (      /* 1:Continue, 0:Abort */
        JDEC* jd,       /* Decompression object */
        void* bitmap,   /* Bitmap data to be output */
        JRECT* rect     /* Rectangular region of output image */
    ) {
        jd_data_t* pData = static_cast<jd_data_t*>(jd->device);

        /* Copy the output image rectanglar to the frame buffer (assuming RGB888 output) */
        uint8_t* rgb = static_cast<uint8_t*>(bitmap);
        uint32_t w = (uint32_t)ceil(pData->fitInfo.scale);
        uint32_t h = w;
        for (uint32_t scanY = rect->top; scanY <= rect->bottom; ++scanY) {
            for (uint32_t scanX = rect->left; scanX <= rect->right; ++scanX) {
                uint32_t x = (uint32_t)ceil(scanX * pData->fitInfo.scale) + pData->fitInfo.offsetX;
                uint32_t y = (uint32_t)ceil(scanY * pData->fitInfo.scale) + pData->fitInfo.offsetY;
                pData->pImage->fillRect(
                    x,
                    y,
                    w,
                    h,
                    rgb_to_gray(rgb[0], rgb[1], rgb[2], 255)
                );
                rgb += 3;
            }
        }

        return 1;    /* Continue to decompress */
    }
}

bool IkedamClock::drawJpeg(M5EPD_Canvas* pImage, Stream* pStream) {
    // http://elm-chan.org/fsw/tjpgd/en/appnote.html
    jd_data_t data = {
        .pImage = pImage,
        .pStream = pStream,
    };
    JDEC jdec;
    uint8_t work[3100];
    JRESULT res = jd_prepare(&jdec, jd_in_func, work, sizeof(work), &data);
    if (res != JDR_OK) {
        log_e("Failed in jd_prepare: %d", res);
        return false;
    }

    data.fitInfo = fit(pImage->width(), pImage->height(), jdec.width, jdec.height);

    res = jd_decomp(&jdec, jd_out_func, JPEG_DIV_NONE);
    if (res != JDR_OK) {
        log_e("Failed in jd_decomp: %d", res);
        return false;
    }

    return true;
}

bool IkedamClock::drawBmp(M5EPD_Canvas* pImage, Stream* pStream) {
    log_e("IkedamClock::drawBmp: Not implemented");
    return false;
}
