/*!
 *	\file  pmos_test.c
 * 
 *  \brief Parametric MOS models for multi-screen systems.
 *
 *	This module implements test program illustrating operations of the psnr2mos() and ssim2mos() functions. 
 * 
 *	\version  1.0.0
 *  \date     Jul 3, 2025
 *	\author   Yuriy A.Reznik, yreznik@streaminglabs.com
 *
 *	\copyright(c) 2025 Streaming Labs, Ltd.
 */

#include <stdio.h>
#include <math.h>
#include "pmos.h"

/* Netflix data set (device: HDTV, SDR, player size = full screen): */
static struct { char* name; int width, height; double psnr, ssim, mos; } dataset[] =
{
    {"s01", 384,  288,  35.620239, 0.959829, 1.3077},
    {"s02", 512,  384,  35.724288, 0.95701,  2.0769},
    {"s03", 512,  384,  36.707835, 0.967129, 2.4615},
    {"s04", 720,  480,  36.75937,  0.96377,  3.1538},
    {"s05", 720,  480,  38.08809,  0.975919, 3.8077},
    {"s06", 1280, 720,  38.401149, 0.972789, 4.6538},
    {"s07", 1280, 720,  39.10971,  0.978547, 4.5769},
    {"s08", 1920, 1080, 38.816344, 0.965304, 4.6538},
    {"s09", 1920, 1080, 39.475595, 0.969544, 4.8846},
    {"s10", 1920, 1080, 41.03835,  0.977687, 4.8077},
    {"s11", 384,  288,  43.470204, 0.990331, 2.1154},
    {"s12", 384,  288,  44.108739, 0.99217,  1.9615},
    {"s13", 512,  384,  44.015573, 0.989149, 2.6538},
    {"s14", 512,  384,  44.553589, 0.990767, 2.8077},
    {"s15", 720,  480,  44.319652, 0.98691, 3.5769},
    {"s16", 1280, 720,  43.634828, 0.977992, 4.4231},
    {"s17", 1920, 1080, 41.336203, 0.954698, 4.8846},
    {"s18", 1920, 1080, 41.71157,  0.957351, 4.8462},
    {"s19", 384,  288,  25.824094, 0.817631, 1},
    {"s20", 720,  480,  28.514402, 0.892381, 1.9231},
    {"s21", 1920, 1080, 27.170572, 0.803234, 2.7692},
    {"s22", 1920, 1080, 28.121391, 0.841992, 3.2692},
    {"s23", 1920, 1080, 28.869742, 0.869312, 3.7308},
    {"s24", 1920, 1080, 29.616956, 0.893737, 4.3462},
    {"s25", 1920, 1080, 30.496047, 0.91915,  4.5},
    {"s26", 384,  288,  30.474125, 0.875135, 1.1923},
    {"s27", 512,  384,  32.35796,  0.903011, 1.5769},
    {"s28", 720,  480,  35.415544, 0.941262, 3.1538},
    {"s29", 1280, 720,  35.009639, 0.92282,  3.4615},
    {"s30", 1920, 1080, 36.314746, 0.931716, 4.1538},
    {"s31", 1920, 1080, 37.811686, 0.947554, 4.6154},
    {"s32", 1920, 1080, 39.115112, 0.958439, 4.6923},
    {"s33", 384,  288,  29.830015, 0.809762, 1.3077},
    {"s34", 720,  480,  31.026768, 0.849293, 2.6154},
    {"s35", 1280, 720,  30.54432,  0.84661,  2.9231},
    {"s36", 1920, 1080, 29.52626,  0.837443, 3.1923},
    {"s37", 1280, 720,  31.662805, 0.871714, 3.6538},
    {"s38", 1920, 1080, 30.533075, 0.860243, 4},
    {"s39", 1920, 1080, 32.631513, 0.901982, 4.3846},
    {"s40", 1920, 1080, 34.7741,   0.931491, 4.6538},
    {"s41", 1920, 1080, 36.557732, 0.949092, 4.7692},
    {"s42", 384,  288,  35.889891, 0.95701,  1.9615},
    {"s43", 512,  384,  37.850343, 0.971129, 3.1923},
    {"s44", 720,  480,  37.063021, 0.960206, 3.3077},
    {"s45", 720,  480,  40.689888, 0.984093, 4.0385},
    {"s46", 1920, 1080, 39.721312, 0.970356, 4.6538},
    {"s47", 1920, 1080, 45.335584, 0.989825, 4.8846},
    {"s48", 384,  288,  35.731982, 0.960807, 1.1154},
    {"s49", 512,  384,  36.446616, 0.963934, 1.8462},
    {"s50", 720,  480,  35.807652, 0.952641, 2.4615},
    {"s51", 720,  480,  37.404842, 0.964863, 3.0769},
    {"s52", 1280, 720,  36.997115, 0.939794, 4.2308},
    {"s53", 1280, 720,  37.302171, 0.942176, 4.3846},
    {"s54", 1920, 1080, 35.337223, 0.881615, 4.7692},
    {"s55", 384,  288,  29.547652, 0.832657, 1.0385},
    {"s56", 720,  480,  30.277566, 0.847808, 2.0769},
    {"s57", 720,  480,  32.265447, 0.899588, 2.6538},
    {"s58", 1280, 720,  30.849491, 0.848564, 3.2308},
    {"s59", 1280, 720,  31.866142, 0.874731, 3.6923},
    {"s60", 1920, 1080, 30.644579, 0.819165, 3.9615},
    {"s61", 1920, 1080, 31.72604,  0.846712, 4.2308},
    {"s62", 1920, 1080, 32.663728, 0.867245, 4.4615},
    {"s63", 1920, 1080, 34.801,    0.903836, 4.3077},
    {"s64", 1920, 1080, 35.522221, 0.9138,   4.5769},
    {"s65", 384,  288,  38.55262,  0.958848, 1.5769},
    {"s66", 512,  384,  40.639229, 0.969591, 2.5769},
    {"s67", 720,  480,  41.28698,  0.970027, 3.2308},
    {"s68", 720,  480,  43.314695, 0.980449, 3.3077},
    {"s69", 1280, 720,  43.641809, 0.977314, 4.2308},
    {"s70", 1920, 1080, 42.476554, 0.96548,  4.5385}
};

 /*!
  *  \brief Main function: test program & demo
  */
int main(int argc, char* argv[])
{
    int n, n_tests = sizeof(dataset) / sizeof(dataset[0]);
    int player_height = 2160, player_width = 3840;  /* assume 4K TV, and player running full screen */
    double mos, delta, rms, scale = 8;

    /*
     * Test PSNR2MOS conversions:
     */
    printf("Testing PSNR2MOS:\n");
    for (n = 0, rms = 0.; n < n_tests; n++) 
    {
        /* predict MOS using model: */
        mos = psnr2mos(dataset[n].psnr, dataset[n].width, dataset[n].height, player_width, player_height, 0, upsampling_bicubic, device_tv, NULL);
        if (mos < 0) { printf("test %d has failed\n", n); return 1; }
        /* compute delta & rms: */
        delta = mos - dataset[n].mos;
        rms += delta * delta;
        /* print result: */
        printf("%s -> %dx%d, PSNR=%g, predicted MOS=%g, true MOS=%g, delta=%g\n", dataset[n].name, dataset[n].width, dataset[n].height, dataset[n].psnr, mos, dataset[n].mos, delta);
    }
    /* report rms: */
    rms = sqrt(rms / (double)n_tests);
    printf("  => rms = %g\n\n", rms);

    /*
     * Test SSIM2MOS conversions:
     */
    printf("Testing SSIM2MOS:\n");
    for (n = 0, rms = 0.; n < n_tests; n++)
    {
        /* predict MOS using model: */
        mos = ssim2mos(dataset[n].ssim, dataset[n].width, dataset[n].height, player_width, player_height, 0, upsampling_bicubic, device_tv, NULL);
        if (mos < 0) { printf("test %d has failed\n", n); return 1; }
        /* compute delta & rms: */
        delta = mos - dataset[n].mos;
        rms += delta * delta;
        /* print result: */
        printf("%s -> %dx%d, SSIM=%g, predicted MOS=%g, true MOS=%g, delta=%g\n", dataset[n].name, dataset[n].width, dataset[n].height, dataset[n].ssim, mos, dataset[n].mos, delta);
    }
    /* report rms: */
    rms = sqrt(rms / (double)n_tests);
    printf("  => rms = %g\n\n", rms);

    return 0;
}

/* pmos_test.c -- end of file */

