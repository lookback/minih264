// clang -g -O1 -fsanitize=address tests/deblock_boundaries.c -lm -o /tmp/deblock-test
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#define H264E_SVC_API 0
#define MINIH264_IMPLEMENTATION
#include "../minih264e.h"

int main(void)
{
    const int sizes[][2] = {{96, 64}, {98, 66}};
    for (unsigned n = 0; n < sizeof(sizes) / sizeof(sizes[0]); n++) {
        const int width = sizes[n][0], height = sizes[n][1];
        const int ys = (width + 15) & ~15, cs = (width / 2 + 15) & ~15;
        H264E_create_param_t create = {
            .width = width, .height = height, .enableNEON = 1,
            .const_input_flag = 1, .vbv_size_bytes = 20000,
        };
        int state_size, scratch_size;
        assert(!H264E_sizeof(&create, &state_size, &scratch_size));
        void *state = NULL, *scratch = NULL, *pixels = NULL;
        assert(!posix_memalign(&state, 64, state_size));
        assert(!posix_memalign(&scratch, 64, scratch_size));
        assert(!posix_memalign(&pixels, 64, (ys + cs) * height));
        memset(pixels, 128, (ys + cs) * height);
        assert(!H264E_init(state, &create));
        H264E_io_yuv_t picture = {
            .yuv = {pixels, (unsigned char *)pixels + ys * height,
                (unsigned char *)pixels + ys * height + cs * height / 2},
            .stride = {ys, cs, cs},
        };
        for (int frame = 0; frame < 4; frame++) {
            H264E_run_param_t run = {
                .frame_type = frame ? H264E_FRAME_TYPE_P : H264E_FRAME_TYPE_KEY,
                .desired_frame_bytes = 2000, .qp_min = 30, .qp_max = 50,
            };
            unsigned char *output;
            int output_size;
            assert(!H264E_encode(state, scratch, &run, &picture, &output, &output_size));
            assert(output_size > 0);
        }
        free(pixels);
        free(scratch);
        free(state);
    }
    return 0;
}
