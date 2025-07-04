/*!
 *  \file  pmos.h
 *  \brief Parametric MOS models for multi-screen systems.
 *
 *  \version  1.0.0
 *  \date     Jul 3, 2025
 *  \author   Yuriy A. Reznik, yreznik@streaminglabs.com
 *
 *  \copyright (c) 2025 Streaming Labs, Ltd.
 */

#ifndef _PMOS_H_
#define _PMOS_H_ 1
#ifdef __cplusplus
extern "C" {
#endif

/*! Device types: */
enum device_types { 
	device_mobile = 0,		/* mobile device, e.g. Samsung Galaxy S21, 6.2", viewing distance ~ 13"*/
	device_tablet,			/* tablet, e.g. Samsung Galaxy Tab S8 Plus, 12.4", viewing distance ~ 18 */
	device_pc,			/* PC screen, e.g. Dell UltraSharp U3011, 30", viewing distance ~ 24" */
	device_tv,			/* TV, e.g. 55" UHDTV set, viewing distance ~ 3H (81") */
	device_custom,			/* custom device -> device_param structure must be provided to define its parameters */
	n_device_types			/* the number of device types defined by this enum */
};

/*! Device parameters: */
struct device_params {
	int display_width;		/* display width [pixels] */
	int display_height;		/* display height [pixels] */
	double ppi_x;			/* display pixel density in horizontal dimension [ppi] */
	double ppi_y;			/* display pixel density in vertical dimension [ppi] */
	int distance_type;		/* viewing distance type: 0 - absolute [in], 1 - relative [heights of the display] */
	double distance;		/* viewing distance [units as indicated by distance_type] */
};

/*! Upsampling methods: */
enum upsampling_methods {
	upsampling_bicubic = 0,		/* bicubic upsampling - commonly used conventional upsampling method [default] */
	upsampling_nn,			/* nearest neighbour usampling - horrible, but occasionally ocurring in practice */
	upsampling_sr,			/* super-resolution based upsampling - indicates the use of more sophisticated algorithm than bicubic */
	n_upsampling_methods		/* the number of upsampling methods defined by this enum */
};

/*! Function prototypes: */
double psnr2mos(double psnr, int width, int height, int player_width, int player_height, int hdr, int upsampling, int device, struct device_params* params);
double ssim2mos(double ssim, int width, int height, int player_width, int player_height, int hdr, int upsampling, int device, struct device_params* params);
double vif2mos(double vif, int width, int height, int player_width, int player_height, int hdr, int upsampling, int device, struct device_params* params);
double vmaf2mos(double vmaf, int width, int height, int player_width, int player_height, int hdr, int upsampling, int device, struct device_params* params);

#ifdef __cplusplus
}
#endif
#endif

